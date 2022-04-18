#pragma once

#include <memory>
#include "render/Buffer/GPUBuffer.h"
#include "render/Buffer/DynamicBuffer.h"
#include "resources/MaterialResource.h"
#include "render/material/Material.h"
#include "render/renderer/DrawCallManager.h"

namespace Device
{
	class Texture;
	class VulkanRenderState;
	class ShaderProgram;
	class ShaderCache;
	class ConstantBindings;
}

class Mesh;

namespace ECS
{
	class EntityManager;
}

namespace ECS::systems
{
	class GPUParticleUpdateSystem;
}

namespace render::graph
{
	class RenderGraph;
	class IRenderPassBuilder;
}

namespace render
{
	class SceneRenderer;
	class DrawCallManager;
	class BitonicSort;
}

namespace render::GPUParticles {

#pragma pack(push, 1)
	struct CounterArgumentsData
	{
		/// DispatchIndirect args for Update
		int32_t groupsX = 0;
		int32_t groupsY = 1;
		int32_t groupsZ = 1;
		//////////////////////////////////////

		/// DrawIndexedIndirect args for Render
		int32_t indexCount = 6;
		int32_t instanceCount = 0;
		int32_t firstIndex = 0;
		int32_t vertexOffset = 0;
		int32_t firstInstance = 0;
		/////////////////////////////////////////

		/// DispatchIndirect args for PreSort
		int32_t preSortGroupsX = 0;
		int32_t preSortGroupsY = 1;
		int32_t preSortGroupsZ = 1;
		//////////////////////////////////////

		// Persistent counters
		int32_t alive_count = 0;
		int32_t dead_count = 0;
		int32_t alive_count_after_simulation = 0;
		//////////////////////

		CounterArgumentsData(int32_t capacity, int32_t index_count)
		{
			dead_count = capacity;
			indexCount = index_count;
		}
	};
#pragma pack(pop)

	class EmitterGPUData
	{
	public:
		struct ExtraBindings
		{
			render::ResourceBindingStorage resources;
			render::ConstantBindingStorage constants;
		};

		struct Initializer
		{
			Initializer(std::string name, uint32_t max_particles = 10000, bool sort = false, render::MaterialUnion material = render::MaterialUnion());
			Initializer& SetShaderPath(std::wstring update_shader, std::wstring emit_shader = L"");
			Initializer& SetMesh(Common::Handle<Mesh> value) { mesh = value; return *this; };
			Initializer& SetExtraBindings(ExtraBindings value) { extra_bindings = std::move(value); return *this; };

			std::string name;
			bool sort = false;
			ExtraBindings extra_bindings;
			std::wstring emit_shader_path;
			std::wstring update_shader_path;
			uint32_t max_particles = 10000;
			render::MaterialUnion material = render::MaterialUnion();
			Common::Handle<Mesh> mesh;
		};

		EmitterGPUData(const Initializer& initializer);
		EmitterGPUData(const EmitterGPUData&) = delete;
		EmitterGPUData(EmitterGPUData&&) = delete;
		EmitterGPUData& operator=(const EmitterGPUData&) = delete;
		EmitterGPUData& operator=(EmitterGPUData&&) = delete;
		~EmitterGPUData();

		void FlushExtraConstants(Device::ConstantBindings& bindings) const;
		void FlushExtraResources(Device::ResourceBindings& bindings) const;

		Device::ShaderProgram* GetUpdateShader() const { return update_shader; }
		Device::ShaderProgram* GetEmitShader() const { return emit_shader; }

		const Device::GPUBuffer& GetParticles() const { return particles; }
		const Device::GPUBuffer& GetDeadIndices() const { return dead_indices; }
		const Device::GPUBuffer& GetAliveIndices() const { return alive_indices; }
		const Device::GPUBuffer& GetDrawIndices() const { return draw_indices; }
		const Device::GPUBuffer* GetSortedIndices() const { return sorted_indices.get(); }
		const Device::DynamicBuffer<uint8_t>& GetCounters() const { return counters; }
		const Common::Handle<Mesh>& GetMesh() const { return mesh; }
		const Common::Handle<Material>& GetMaterial() const { return material.Get(); }

		bool GetSort() const { return sort; }
		uint32_t GetMaxParticles() const { return max_particles; }

	private:
		friend class GPUParticles;
		void SetActualMaterial(Common::Handle<Material>& value) { actual_material = value; }

		std::vector<Device::ShaderProgramInfo::Macro> GetMacros() const;

	private:
		uint32_t max_particles = 10000;
		bool sort = false;
		render::DrawCallManager::Handle draw_calls;
		Device::GPUBuffer particles;
		Device::GPUBuffer dead_indices;
		Device::GPUBuffer alive_indices;
		Device::GPUBuffer draw_indices;
		std::unique_ptr<Device::GPUBuffer> sorted_indices;
		Device::DynamicBuffer<uint8_t> counters;
		ExtraBindings extra_bindings;
		Common::Handle<Mesh> mesh;
		render::MaterialUnion material;
		Common::Handle<Material> actual_material;


		Device::ShaderProgram* emit_shader = nullptr;
		Device::ShaderProgram* update_shader = nullptr;
	};

	class GPUParticles : public NonCopyable
	{
	public:
		GPUParticles(SceneRenderer& scene_renderer, BitonicSort& bitonic_sort, ECS::EntityManager& manager);
		~GPUParticles();

		void Update(graph::RenderGraph& graph, const Device::ConstantBindings& global_constants, float dt);
		const EmitterGPUData* CreateGPUData(const EmitterGPUData::Initializer& initializer) const;
	private:
		void UpdateDrawCalls(EmitterGPUData& gpu_data, float dt);
		void ComputeParticlePreUpdate(EmitterGPUData& gpu_data, Device::VulkanRenderState& state);
		void ComputeParticleUpdate(EmitterGPUData& gpu_data, const Device::ConstantBindings& global_constants, Device::VulkanRenderState& state, float dt);

	private:
		mutable std::unordered_set<std::unique_ptr<EmitterGPUData>> gpu_datas;
		std::unique_ptr<ECS::systems::GPUParticleUpdateSystem> particle_update_system;
		SceneRenderer& scene_renderer;
		BitonicSort& bitonic_sort;
		ECS::EntityManager& manager;

		Device::ShaderProgram* pre_sort_shader = nullptr;
		Device::ShaderProgram* pre_sort_args_shader = nullptr;
		Device::ShaderProgram* output_sorted_shader = nullptr;
	};

}