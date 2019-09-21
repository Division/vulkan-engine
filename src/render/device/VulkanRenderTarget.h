#pragma once

#include "CommonIncludes.h"

namespace core { namespace Device {

	class VulkanRenderPass;
	class VulkanSwapchain;

	struct VulkanRenderTargetInitializer {

		VulkanRenderTargetInitializer(VulkanRenderPass* render_pass)
		{
			if (!render_pass)
				throw std::runtime_error("Render pass should be specified");

			this->render_pass = render_pass;
			this->use_swapchain = use_swapchain;
		}

		VulkanRenderTargetInitializer& Swapchain(VulkanSwapchain* swapchain)
		{
			use_swapchain = true;
			this->swapchain = swapchain;
			return *this;
		}

		VulkanRenderTargetInitializer& Size(uint32_t width, uint32_t height) 
		{
			this->width = width;
			this->height = height;
			return *this;
		}

		VulkanRenderTargetInitializer& ColorTarget(uint32_t sampleCount = 1, vk::Format colorFormat = vk::Format::eB8G8R8A8Snorm) 
		{
			has_color = true;
			this->sample_count = sampleCount;
			this->color_format = colorFormat;
			return *this;
		}

		VulkanRenderTargetInitializer& DepthTarget(bool bindShaderResource, vk::Format depthFormat = vk::Format::eD24UnormS8Uint) 
		{
			has_depth = true;
			this->depth_format = depthFormat;
			return *this;
		}

		uint32_t sample_count = 1;
		uint32_t width = 0;
		uint32_t height = 0;
		bool use_swapchain = false;
		bool has_color = false;
		bool has_depth = false;
		VulkanSwapchain* swapchain = nullptr;
		VulkanRenderPass* render_pass;
		vk::Format color_format = vk::Format::eB8G8R8A8Snorm;
		vk::Format depth_format = vk::Format::eD24UnormS8Uint;
	};

	class VulkanRenderTarget
	{
	public:
		struct Frame
		{
			vk::Image target;
			vk::UniqueImageView image_view;
			vk::UniqueFramebuffer framebuffer;
		};

		VulkanRenderTarget(VulkanRenderTargetInitializer initializer);

		void Resize(uint32_t width, uint32_t height);
		const Frame& GetFrame(int index) const { return frames.at(index); }
		uint32_t GetWidth() const { return  width; }
		uint32_t GetHeight() const { return  height; }
		VulkanRenderPass* GetRenderPass() const { return render_pass; }

	private:
		std::vector<Frame> frames;
		uint32_t sample_count = 1;
		uint32_t width = 0;
		uint32_t height = 0;
		bool use_swapchain = false;
		VulkanSwapchain* swapchain;
		bool has_color = false;
		bool has_depth = false;
		VulkanRenderPass* render_pass;
		vk::Format color_format = vk::Format::eB8G8R8A8Snorm;
		vk::Format depth_format = vk::Format::eD24UnormS8Uint;
	};

} }