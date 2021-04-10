#pragma once

#include "scene/Scene.h"
#include "CommonIncludes.h"
#include "render/buffer/DynamicBuffer.h"
#include "render/shader/ShaderBufferStruct.h"

class ICameraParamsProvider;

class LightObject;
class Projector;

namespace ECS::components
{
	class Light;
}

namespace render {

	struct LightGridCell {
		int offset;
		std::vector<std::shared_ptr<LightObject>> pointLights;
		std::vector<std::shared_ptr<LightObject>> spotLights;
		std::vector<std::shared_ptr<Projector>> projectors;
		std::vector<std::shared_ptr<Projector>> decals;
	};

	struct LightGridStruct {
		uint32_t offset;
		uint16_t pointLightCount;
		uint16_t spotLightCount;
		uint16_t projectorCount;
		uint16_t decalCount;
	};

	constexpr uint32_t CLUSTER_COUNT_X = 4;
	constexpr uint32_t CLUSTER_COUNT_Y = 4;
	constexpr uint32_t CLUSTER_COUNT_DEPTH = 16;
	constexpr float CLUSTER_NEAR = 0.1f;
	constexpr float CLUSTER_FAR = 50000.0f;

	struct ClusterSlice
	{
		struct ClusterInfo
		{
			std::array<vec3, 8> view_space_frustum_corners;
			AABB viewspace_aabb; // Cluster frustum is an AABB in NDC
		};

		Memory::Vector<uint32_t, Memory::Tag::Render> indices;
		std::array<ClusterInfo, CLUSTER_COUNT_X * CLUSTER_COUNT_Y> clusters_info;
		std::array<LightGridStruct, CLUSTER_COUNT_X * CLUSTER_COUNT_Y> clusters;
	};

	class LightGrid {
	public:
		static float GetSliceMaxDepth(uint32_t slice);
		static uint32_t GetSliceIndex(float depth);

		explicit LightGrid();
		~LightGrid() = default;

		void appendLights(const std::vector<Scene::SceneLightData> &lights, ICameraParamsProvider* camera, float shadowmap_atlas_size);
		void upload();
		Device::DynamicBuffer<Device::ShaderBufferStruct::Light>* GetLightsBuffer() const { return lights[0].get(); };
		Device::DynamicBuffer<Device::ShaderBufferStruct::Projector>* GetProjectorBuffer() const { return projectors[0].get(); };
		Device::DynamicBuffer<char>* GetLightIndexBuffer() const { return light_index[0].get(); };
		Device::DynamicBuffer<char>* GetLightGridBuffer() const { return light_grid[0].get(); };

		void DrawDebugClusters(mat4 model_matrix, vec4 color);
		void OnRecreateSwapchain(int32_t width, int32_t height);
		void UpdateSlices(mat4 projection);

	private:
		Memory::Vector<ClusterSlice, Memory::Tag::Render> slices;
		std::unique_ptr<Device::DynamicBuffer<Device::ShaderBufferStruct::Light>> lights[2];
		std::unique_ptr<Device::DynamicBuffer<Device::ShaderBufferStruct::Projector>> projectors[2];
		std::unique_ptr<Device::DynamicBuffer<char>> light_index[2];
		std::unique_ptr<Device::DynamicBuffer<char>> light_grid[2];

	private:
		mat4 last_matrix;
		Memory::Vector<AABB, Memory::Tag::Render> light_aabb;
		Memory::Vector<AABB, Memory::Tag::Render> projector_aabb;
	};

}