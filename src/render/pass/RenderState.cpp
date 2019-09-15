#include "RenderState.h"
#include "render/device/VkObjects.h"

namespace core { namespace render {

	RenderState::RenderState()
		: command_pool(std::make_unique<core::Device::VulkanCommandPool>())
	{

	}

	RenderState::~RenderState()
	{

	}
	
	void RenderState::BeginRendering()
	{

	}

	void RenderState::EndRendering()
	{

	}

} }