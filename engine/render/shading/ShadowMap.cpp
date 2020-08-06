#include "Engine.h"
#include "ShadowMap.h"
#include "IShadowCaster.h"
#include "render/texture/Texture.h"
#include "render/device/VulkanRenderState.h"
#include "ecs/systems/CullingSystem.h"

namespace render {

	static const unsigned int CELL_COUNT = 4;
	static const auto MAX_MAPS = CELL_COUNT * CELL_COUNT;

	ShadowMap::ShadowMap(unsigned int resolutionX, unsigned int resolutionY) {
	  _resolution = uvec2(resolutionX, resolutionY);

	  float emptySpacing = (float)((CELL_COUNT - 1u) * _pixelSpacing);
	  _cellPixelSize = glm::floor(vec2(resolutionX - emptySpacing, resolutionY - emptySpacing) / (float)CELL_COUNT);
	  _cellSize = vec2(_cellPixelSize) / vec2(_resolution);
	}

	void ShadowMap::SetupShadowCasters(std::vector<std::pair<IShadowCaster*, ECS::systems::CullingSystem>>& shadow_casters) {
		unsigned int index = 0;

		for (auto& caster : shadow_casters) {
			vec4 viewport = (vec4)getCellPixelRect(index);
			caster.first->viewport(viewport);
			index++;
		}
	}

	Rect ShadowMap::getCellPixelRect(unsigned int index) {
	  unsigned int x = index % CELL_COUNT * (_cellPixelSize.x + _pixelSpacing);
	  unsigned int y = index / CELL_COUNT * (_cellPixelSize.y + _pixelSpacing);

	  return Rect((float)x, (float)y, (float)_cellPixelSize.x, (float)_cellPixelSize.y);
	}

	Rect ShadowMap::getCellRect(unsigned int index) {
	  int x = index % CELL_COUNT;
	  int y = index / CELL_COUNT;

	  vec2 origin = vec2(x, y) * (vec2(_cellPixelSize) + (float)_pixelSpacing) / vec2(_resolution);

	  return Rect(origin.x, origin.y, _cellSize.x, _cellSize.y);
	}

}