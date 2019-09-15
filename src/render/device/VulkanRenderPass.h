#pragma once

#include "CommonIncludes.h"

namespace core { namespace Device {

	struct VulkanRenderPassInitializer {

		VulkanRenderPassInitializer(vk::Format format) : format(format) {}

		vk::Format format;
	};

	class VulkanRenderPass
	{
	public:
		VulkanRenderPass(const VulkanRenderPassInitializer& initializer);

		vk::RenderPass GetRenderPass() const { return render_pass.get(); }

	private:
		vk::UniqueRenderPass render_pass;
	};

} }