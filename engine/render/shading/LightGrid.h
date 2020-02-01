#pragma once

#include "CommonIncludes.h"
#include "render/buffer/DynamicBuffer.h"
#include "render/shader/ShaderBufferStruct.h"

class ICameraParamsProvider;

class LightObject;
class Projector;

namespace core { namespace render {

	using namespace core::Device;

	struct LightGridCell {
		int offset;
		std::vector<std::shared_ptr<LightObject>> pointLights;
		std::vector<std::shared_ptr<LightObject>> spotLights;
		std::vector<std::shared_ptr<Projector>> projectors;
		std::vector<std::shared_ptr<Projector>> decals;
	};

	class LightGrid {
	public:
		explicit LightGrid(unsigned int cellSize = 32);
		~LightGrid() = default;

		void Update(unsigned int screenWidth, unsigned int screenHeight);
		void appendLights(const std::vector<std::shared_ptr<LightObject>> &lights, ICameraParamsProvider* camera);
		void appendProjectors(const std::vector<std::shared_ptr<Projector>> &projectors, ICameraParamsProvider* camera);
		void upload();
		DynamicBuffer<ShaderBufferStruct::Light>* GetLightsBuffer() const { return lights[0].get(); };
		DynamicBuffer<ShaderBufferStruct::Projector>* GetProjectorBuffer() const { return projectors[0].get(); };
		DynamicBuffer<char>* GetLightIndexBuffer() const { return light_index[0].get(); };
		DynamicBuffer<char>* GetLightGridBuffer() const { return light_grid[0].get(); };

		void OnRecreateSwapchain(int32_t width, int32_t height);

	private:
		std::unique_ptr<DynamicBuffer<ShaderBufferStruct::Light>> lights[2];
		std::unique_ptr<DynamicBuffer<ShaderBufferStruct::Projector>> projectors[2];
		std::unique_ptr<DynamicBuffer<char>> light_index[2];
		std::unique_ptr<DynamicBuffer<char>> light_grid[2];

		uint32_t _lightCount;
		uint32_t _projectorCount;

		unsigned int _cellSize;
		unsigned int _cellsX = 0;
		unsigned int _cellsY = 0;
		std::vector<LightGridCell> _cells;
		unsigned int _lightGridBlock;
		unsigned int _lightIndexBlock;

	private:
		LightGridCell *_getCellByXY(int x, int y) { return &_cells[x + y * _cellsX]; };
		void _clearCells();
		void _appendItem(ICameraParamsProvider* camera, const std::vector<vec3> &edgePoints,
						 std::function<void(LightGridCell *)> callback);

		// Temporary vector to pass data through the functions
		// Placed here instead of the stack to reduce heap allocations
		std::vector<vec3> _lightEdges;
		std::vector<char> temp_data;
	};

} }
