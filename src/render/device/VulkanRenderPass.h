#pragma once

#include "CommonIncludes.h"

namespace core { namespace Device {

	struct VulkanRenderPassInitializer {

		VulkanRenderPassInitializer(vk::Format format, bool has_depth) : format(format), has_depth(has_depth) {}

		vk::Format format;
		bool has_depth;
	};

	class VulkanRenderPass
	{
	public:
		VulkanRenderPass(const VulkanRenderPassInitializer& initializer);

		vk::RenderPass GetRenderPass() const { return render_pass.get(); }

		bool HasDepth() const { return has_depth; }
		vk::Format GetDepthFormat() const { return depth_format; }

	private:
		vk::UniqueRenderPass render_pass;
		bool has_depth;
		vk::Format depth_format = vk::Format::eD24UnormS8Uint;
	};

} }