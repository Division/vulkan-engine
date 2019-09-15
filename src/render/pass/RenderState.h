#pragma once

#include "CommonIncludes.h"

namespace core { namespace Device {
	class VulkanCommandPool;
} }

namespace core { namespace render {

	enum class BlendMode
	{
		Opaque,
		Translucent
	};

	class RenderState : NonCopyable
	{
	public:
		RenderState();
		~RenderState();

		void BeginRendering();
		void EndRendering();
		
	private:
		std::unique_ptr<core::Device::VulkanCommandPool> command_pool;
	};

} }