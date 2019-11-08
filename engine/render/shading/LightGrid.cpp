#include <functional>

#include "LightGrid.h"
#include "objects/Camera.h"
#include "objects/LightObject.h"
#include "objects/Projector.h"
#include "system/Logging.h"
#include "render/renderer/ICameraParamsProvider.h"
#include "render/device/VulkanContext.h"
#include "render/shader/ShaderResource.h"
#include "render/buffer/UniformBuffer.h"

namespace core { namespace render {

const uint32_t MAX_LIGHTS = 100;
const uint32_t LIGHTS_SIZE = ((uint32_t)ceilf(MAX_LIGHTS * sizeof(ShaderBufferStruct::Light) / 256.0f)) * 256;
const uint32_t PROJECTORS_SIZE = ((uint32_t)ceilf(MAX_LIGHTS * sizeof(ShaderBufferStruct::Light) / 256.0f)) * 256;

struct LightGridStruct {
	uint32_t offset;
	uint16_t pointLightCount;
	uint16_t spotLightCount;
	uint16_t projectorCount;
	uint16_t decalCount;
};

LightGrid::LightGrid(unsigned int cellSize) : _cellSize(cellSize) 
{	
	auto* context = Engine::GetVulkanContext();
	context->AddRecreateSwapchainCallback(std::bind(&LightGrid::OnRecreateSwapchain, this, std::placeholders::_1, std::placeholders::_2));
}

void LightGrid::Update(unsigned int screenWidth, unsigned int screenHeight) {
  auto newCellsX = (unsigned int)ceilf((float)screenWidth / (float)_cellSize);
  auto newCellsY = (unsigned int)ceilf((float)screenHeight / (float)_cellSize);

  if (newCellsX != _cellsX || newCellsY != _cellsY) {
    _cellsX = newCellsX;
    _cellsY = newCellsY;
  }

  auto size = _cellsX * _cellsY;
  if (size > _cells.size()) {
    _cells.resize(_cellsX * _cellsY);
  }

  _clearCells();
}

void LightGrid::_clearCells() {
	for (auto &cell: _cells) {
		cell.pointLights.clear();
		cell.spotLights.clear();
		cell.projectors.clear();
		cell.decals.clear();
	}
}

template <typename T>
void ResizeBuffer(std::unique_ptr<UniformBuffer<T>> buffer[2], size_t size, bool is_storage)
{
	if (!buffer[0] || buffer[0]->GetSize() < size)
	{
		buffer[1] = std::move(buffer[0]);
		buffer[0] = std::make_unique<UniformBuffer<T>>(size, is_storage, false);
	}
}

// Executes callback for every cell within projected edgePoints bounds
// edgePoints is vector because it has different length depending on the object
void LightGrid::_appendItem(ICameraParamsProvider* camera, const std::vector<vec3> &edgePoints,
                            std::function<void(LightGridCell *)> callback) {
  std::vector<vec3> projectedEdges(edgePoints.size());
  AABB bounds;
  
  for (int i = 0; i < edgePoints.size(); i++) {
    projectedEdges[i] = glm::project(edgePoints[i], camera->cameraViewMatrix(), camera->cameraProjectionMatrix(), camera->cameraViewport());
    if (i == 0) {
      bounds.min = projectedEdges[i];
      bounds.max = projectedEdges[i];
    } else {
      bounds.expand(projectedEdges[i]);
    }
  }

  float cellSize = (float)_cellSize;
  float lastCellX = (float)((int)_cellsX - 1);
  float lastCellY = (float)((int)_cellsY - 1);
  int extra = 3;
  auto startX = (int)round(floorf(fminf(fmaxf(bounds.min.x / cellSize - extra, 0), lastCellX)));
  auto startY = (int)round(floorf(fminf(fmaxf(bounds.min.y / cellSize - extra, 0), lastCellY)));
  auto endX = (int)round(floorf(fmaxf(fminf(bounds.max.x / cellSize + extra, lastCellX), 0)));
  auto endY = (int)round(floorf(fmaxf(fminf(bounds.max.y / cellSize + extra, lastCellY), 0)));

  if ((endX < 0) || (startX >= (int)_cellsX) || (endY < 0) || (startY >= (int)_cellsY)) {
    return; // light out of grid bounds
  }

  for (auto i = startX; i <= endX; i++ ) {
    for (auto j = startY; j <= endY; j++) {
      auto cell = _getCellByXY(i, j);
      callback(cell);
    }
  }
  
}

void LightGrid::appendLights(const std::vector<LightObjectPtr> &light_list,
                             ICameraParamsProvider* camera) {
  _lightCount = light_list.size();
  ResizeBuffer(lights, light_list.size() * sizeof(ShaderBufferStruct::Light), false);
  lights[0]->Map();

  for (int i = 0; i < light_list.size(); i++) {
	  auto &light = light_list[i];
	  auto lightData = light->getLightStruct();
	  lights[0]->Append(lightData);
	  light->index(i);
  }

  _lightEdges.resize(4);

  for (auto &light : light_list) {
    vec3 position = light->transform()->worldPosition();
    float radius = light->radius();

    _lightEdges[0] = position + camera->cameraLeft() * radius;
    _lightEdges[1] = position + camera->cameraRight() * radius;
    _lightEdges[2] = position + camera->cameraUp() * radius;
    _lightEdges[3] = position + camera->cameraDown() * radius;

    _appendItem(camera, _lightEdges, [&](LightGridCell *cell) {
      switch(light->type()) {
        case LightObjectType::Spot:
          cell->spotLights.push_back(light);
          break;
        case LightObjectType::Point:
          cell->pointLights.push_back(light);
          break;
      }
    });
  }

  lights[0]->Unmap();
}

void LightGrid::appendProjectors(const std::vector<std::shared_ptr<Projector>> &projectors_list, ICameraParamsProvider* camera) {

  _projectorCount = projectors_list.size();
  ResizeBuffer(projectors, projectors_list.size() * sizeof(ShaderBufferStruct::Projector), false);
  projectors[0]->Map();

  for (int i = 0; i < projectors_list.size(); i++) {
	  auto &projector = projectors_list[i];
	  auto projectorData = projector->getProjectorStruct();
	  projectors[0]->Append(projectorData);
	  projector->index(i);
  }

  for (auto &projector : projectors_list) {
    projector->getEdgePoints(_lightEdges);

    _appendItem(camera, _lightEdges, [&](LightGridCell *cell) {
      switch(projector->type()) {
        case ProjectorType::Projector:
          cell->projectors.push_back(projector);
          break;
        case ProjectorType::Decal:
          cell->decals.push_back(projector);
          break;
      }
    });
  }

  projectors[0]->Unmap();
}

// Upload grid data into the GPU buffers
void LightGrid::upload() 
{
	ResizeBuffer(light_grid, _cells.size() * sizeof(LightGridStruct), true);

	// Little bit unsafe but convenient way to directly modify data within the memory
	auto gridBufferPointer = (LightGridStruct *)light_grid[0]->Map();
	uint32_t currentOffset = 0;

	for (int i = 0; i < _cells.size(); i++) {
		auto &cell = _cells[i];

		// Writing cell data
		// Referencing memory at the offset sizeof(LightGridStruct) * i
		gridBufferPointer[i].offset = currentOffset;
		gridBufferPointer[i].pointLightCount = (uint16_t)cell.pointLights.size();
		gridBufferPointer[i].spotLightCount = (uint16_t)cell.spotLights.size();
		gridBufferPointer[i].projectorCount = (uint16_t)cell.projectors.size();
		gridBufferPointer[i].decalCount = (uint16_t)cell.decals.size();

		// Writing indices
		// Count of light sources to put into the index data structure
		int indexDataSize = gridBufferPointer[i].pointLightCount +
							gridBufferPointer[i].spotLightCount +
							gridBufferPointer[i].projectorCount +
							gridBufferPointer[i].decalCount;

		temp_data.resize((indexDataSize + currentOffset) * sizeof(uint32_t));
		// pointer should be obtained after resize() since resize may reallocate the data
		auto indexBufferPointer = (uint32_t *)temp_data.data();

		// Indices for point lights
		for (int j = 0; j < gridBufferPointer[i].pointLightCount; j++) {
			indexBufferPointer[currentOffset + j] = (uint32_t)cell.pointLights[j]->index();
		}
		currentOffset += gridBufferPointer[i].pointLightCount;

		// Indices for spot lights
		for (int j = 0; j < gridBufferPointer[i].spotLightCount; j++) {
			indexBufferPointer[currentOffset + j] = (uint32_t)cell.spotLights[j]->index();
		}
		currentOffset += gridBufferPointer[i].spotLightCount;

		// Indices for projectors
		for (int j = 0; j < gridBufferPointer[i].projectorCount; j++) {
			indexBufferPointer[currentOffset + j] = (uint32_t)cell.projectors[j]->index();
		}
		currentOffset += gridBufferPointer[i].projectorCount;

		// Indices for decals
		for (int j = 0; j < gridBufferPointer[i].decalCount; j++) {
			indexBufferPointer[currentOffset + j] = (uint32_t)cell.decals[j]->index();
		}
		currentOffset += gridBufferPointer[i].decalCount;
	}
	light_grid[0]->Unmap();

	ResizeBuffer(light_index, temp_data.size(), true);
	auto light_index_pointer = (char*)light_index[0]->Map();
	memcpy(light_index_pointer, temp_data.data(), temp_data.size());
	light_index[0]->Unmap();
}

void LightGrid::OnRecreateSwapchain(int32_t width, int32_t height)
{
}

} }