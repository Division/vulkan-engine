#pragma once

#include "CommonIncludes.h"
#include "VulkanCaps.h"

namespace core { namespace Device {

	class VulkanRenderPass;
	class VulkanSwapchain;
	class Texture;

	struct VulkanRenderTargetInitializer {

		VulkanRenderTargetInitializer(uint32_t width, uint32_t height)
		{
			Size(width, height);
		}

		VulkanRenderTargetInitializer(VulkanSwapchain* swapchain)
		{
			use_swapchain = true;
			this->swapchain = swapchain;
		}

		VulkanRenderTargetInitializer& Size(uint32_t width, uint32_t height) 
		{
			this->width = width;
			this->height = height;
			return *this;
		}

		VulkanRenderTargetInitializer& ColorTarget(uint32_t sampleCount = 1, Format colorFormat = Format::R8G8B8A8_norm) 
		{
			has_color = true;
			this->sample_count = sampleCount;
			this->color_format = colorFormat;
			return *this;
		}

		VulkanRenderTargetInitializer& DepthTarget(Format depthFormat = Format::D24_unorm_S8_uint)
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
		Format color_format = Format::R8G8B8A8_norm;
		Format depth_format = Format::D24_unorm_S8_uint;
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

		const Frame& GetFrame(int index) const { return frames.at(index); }
		uint32_t GetWidth() const { return  width; }
		uint32_t GetHeight() const { return  height; }

	private:
		std::vector<Frame> frames;
		uint32_t sample_count = 1;
		uint32_t width = 0;
		uint32_t height = 0;
		bool use_swapchain = false;
		VulkanSwapchain* swapchain;
		bool has_color = false;
		bool has_depth = false;
		std::shared_ptr<Texture> depth_texture;
		std::array<std::shared_ptr<Texture>, caps::MAX_FRAMES_IN_FLIGHT> color_textures;
		std::array<vk::Image, caps::MAX_FRAMES_IN_FLIGHT> color_texture_images;

		Format color_format;
		Format depth_format;
	};

} }