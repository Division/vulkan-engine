#pragma once

#include "render/device/VulkanRenderState.h"
#include "IRenderer.h"

namespace render 
{
	Device::RenderMode GetRenderModeForQueue(RenderQueue queue);

	extern const std::unordered_map<std::string, RenderQueue> RENDER_QUEUE_NAME_MAP;
}