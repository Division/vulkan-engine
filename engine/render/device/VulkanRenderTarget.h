#pragma once

#include "CommonIncludes.h"
#include "VulkanCaps.h"
#include "Types.h"

#include "utils/Math.h"

namespace core { namespace Device {

	class VulkanRenderPass;
	class VulkanSwapchain;
	class Texture;

	class VulkanRenderTargetAttachment
	{
	public:
		enum class Type : int
		{
			Color,
			Depth
		};

		struct Frame
		{
			vk::ImageView image_view;
			vk::UniqueImageView swapchain_image_view;
			std::shared_ptr<Texture> color_texture;
		};

		VulkanRenderTargetAttachment(Type type, uint32_t width, uint32_t height, Format format, uint32_t sample_count = 1);
		VulkanRenderTargetAttachment(VulkanSwapchain* swapchain);

		Format GetFormat() const { return format; }
		Type GetType() const { return type; }

	private:
		Type type;
		Format format;
		uint32_t width;
		uint32_t height;

		VulkanSwapchain* swapchain = nullptr;
		std::shared_ptr<Texture> depth_texture;
		std::array<Frame, caps::MAX_FRAMES_IN_FLIGHT> frames;
	};

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

		VulkanRenderTargetInitializer& ColorTarget(std::shared_ptr<Texture> texture);
		VulkanRenderTargetInitializer& DepthTarget(std::shared_ptr<Texture> texture);

		VulkanRenderTargetInitializer& DepthTarget(Format depthFormat = Format::D24_unorm_S8_uint)
		{
			has_depth = true;
			this->depth_format = depthFormat;
			return *this;
		}


		uint32_t GetHash() const
		{
			std::array<uint32_t, 10> hashes;
			memset(hashes.data(), 0, sizeof(hashes));

			return FastHash(hashes.data(), sizeof(hashes));
		}

		uint32_t sample_count = 1;
		uint32_t width = 0;
		uint32_t height = 0;
		bool use_swapchain = false;
		bool has_color = false;
		bool has_depth = false;
		VulkanSwapchain* swapchain = nullptr;
		
		std::shared_ptr<Texture> color_attachment;
		std::shared_ptr<Texture> depth_attachment;

		Format color_format = Format::R8G8B8A8_norm;
		Format depth_format = Format::D24_unorm_S8_uint;
	};

	class VulkanRenderTarget
	{
	public:
		struct Frame
		{
			vk::ImageView image_view;
			vk::UniqueImageView swapchain_image_view;
			vk::UniqueFramebuffer framebuffer;
		};

		VulkanRenderTarget(const VulkanRenderTargetInitializer& initializer);

		const Frame& GetFrame(int index) const { return frames.at(index); }
		uint32_t GetWidth() const { return  width; }
		uint32_t GetHeight() const { return  height; }
		bool IsSwapchain() const { return swapchain != nullptr; }
		bool HasColor() const { return has_color; }
		bool HasDepth() const { return has_depth; }
		Format GetColorFormat() const { return color_format; }
		Format GetDepthFormat() const { return depth_format; }

	private:
		std::vector<Frame> frames;
		uint32_t sample_count = 1;
		uint32_t width = 0;
		uint32_t height = 0;
		bool use_swapchain = false;
		VulkanSwapchain* swapchain = nullptr;
		bool has_color = false;
		bool has_depth = false;
		std::shared_ptr<Texture> depth_texture;
		std::array<std::shared_ptr<Texture>, caps::MAX_FRAMES_IN_FLIGHT> color_textures;
		std::array<vk::Image, caps::MAX_FRAMES_IN_FLIGHT> color_texture_images;

		Format color_format;
		Format depth_format;
	};

} }