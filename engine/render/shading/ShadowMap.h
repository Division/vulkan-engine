#pragma once

#include "CommonIncludes.h"
#include "render/shading/IShadowCaster.h"
#include "utils/Math.h"
#include "render/renderer/DrawCall.h"

class Renderer;
class IShadowCaster;

namespace core
{
	namespace Device
	{
		class VulkanRenderState;
	}

	namespace ECS
	{
		namespace systems
		{
			class CullingSystem;
		}
	}
}

namespace core { namespace render {
	
	class ShadowMap {
	public:
		ShadowMap(unsigned int resolutionX, unsigned int resolutionY);

		void SetupShadowCasters(std::vector<std::pair<IShadowCaster*, core::ECS::systems::CullingSystem>>& shadow_casters);
	
	private:
		uvec2 _resolution;
		uvec2 _cellPixelSize;
		vec2 _cellSize;
		unsigned int _pixelSpacing = 2;
		uint32_t _shadowmapBlock;
		int32_t _shadowCasterCount;

	private:
		Rect getCellPixelRect(unsigned int index);
		Rect getCellRect(unsigned int index);
	};

} }