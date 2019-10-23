#include "Texture.h"
#include "system/Logging.h"
#include "Engine.h"
#include "render/device/Types.h"
#include "render/device/VulkanContext.h"
#include "render/device/VulkanUploader.h"
#include "render/buffer/VulkanBuffer.h"

namespace core { namespace Device {

	Texture::Texture(const TextureInitializer& initializer)
	{
		auto allocator = Engine::GetVulkanContext()->GetAllocator();
		auto& device = Engine::GetVulkanDevice();

		SetupDefault(initializer);
	}

	vk::ImageViewCreateInfo GetImageViewCreateInfo(vk::Image& image, const TextureInitializer& initializer)
	{
		auto image_view_info = vk::ImageViewCreateInfo({}, image, vk::ImageViewType::e2D, vk::Format(initializer.format));
		if (initializer.mode == TextureInitializer::DepthBuffer)
			image_view_info.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1));
		else
			image_view_info.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

		image_view_info.setComponents(vk::ComponentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA));

		return image_view_info;
	}

	VkImageCreateInfo GetImageCreateInfo(const TextureInitializer& initializer)
	{
		VkImageCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		create_info.pNext = NULL;
		create_info.imageType = VK_IMAGE_TYPE_2D;
		create_info.format = VkFormat(initializer.format);
		create_info.extent.width = initializer.width;
		create_info.extent.height = initializer.height;
		create_info.extent.depth = initializer.depth;
		create_info.mipLevels = 1;
		create_info.arrayLayers = initializer.array_layers;
		create_info.samples = VK_SAMPLE_COUNT_1_BIT;
		create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		auto transfer_flags = initializer.mode == TextureInitializer::Default ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0;
		create_info.usage = initializer.mode == TextureInitializer::DepthBuffer ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : (transfer_flags | VK_IMAGE_USAGE_SAMPLED_BIT);
		create_info.queueFamilyIndexCount = 0;
		create_info.pQueueFamilyIndices = NULL;
		create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.flags = 0;

		return create_info;
	}

	std::vector<vk::BufferImageCopy> Texture::GetCopies()
	{
		vk::BufferImageCopy region = vk::BufferImageCopy(0, 0, 0)
			.setImageSubresource(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1))
			.setImageOffset(vk::Offset3D({ 0, 0, 0 }))
			.setImageExtent({ width, height, depth });

		std::vector<vk::BufferImageCopy> copies;
		copies.push_back(region);

		return copies;
	}

	void Texture::SetupDefault(const TextureInitializer& initializer)
	{
		width = initializer.width;
		height = initializer.height;
		array_layers = initializer.array_layers;
		depth = initializer.depth;
		format = initializer.format;
		size = width * height * depth * (uint32_t)format_sizes.at(format) / 8;

		auto allocator = Engine::GetVulkanContext()->GetAllocator();
		auto& device = Engine::GetVulkanDevice();

		VkImageCreateInfo create_info = GetImageCreateInfo(initializer);
		VmaAllocationCreateInfo allocation_create_info = {};
		allocation_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		allocation_create_info.flags = 0;
		VkImage vk_image;

		auto result = vmaCreateImage(allocator, &create_info, &allocation_create_info, &vk_image, &allocation, nullptr);
		if (result == VK_SUCCESS)
		{
			image = vk_image;
		} else 
			throw std::runtime_error("can't allocate image");

		auto image_view_info = GetImageViewCreateInfo(image, initializer);
		image_view = device.createImageViewUnique(image_view_info);

		dynamic = initializer.dynamic;
		if (dynamic)
		{
			auto staging_initializer = VulkanBufferInitializer(size).SetStaging();
			for (int i = 0; i < caps::MAX_FRAMES_IN_FLIGHT; i++)
			{
				staging_buffers[i] = std::make_unique<VulkanBuffer>(staging_initializer);
			}
		}

		if (initializer.data)
		{
			auto* uploader = Engine::GetVulkanContext()->GetUploader();
			auto upload_staging_buffer = std::make_unique<VulkanBuffer>(
				VulkanBufferInitializer(size).SetStaging().Data(initializer.data)
			);
			uploader->AddImageToUpload(std::move(upload_staging_buffer), image, 1, array_layers, GetCopies());
		}
	}

	void* Texture::Map()
	{
		if (!staging_buffers[current_staging_buffer])
			throw new std::runtime_error("texture must be dynamic to be mapped");

		mapped_pointer = staging_buffers[current_staging_buffer]->Map();
		return mapped_pointer;
	}

	void Texture::Unmap()
	{
		staging_buffers[current_staging_buffer]->Unmap();
		auto* uploader = Engine::GetVulkanContext()->GetUploader();
		uploader->AddImageToUpload(staging_buffers[current_staging_buffer].get(), image, 1, array_layers, GetCopies());

		current_staging_buffer = (current_staging_buffer + 1) % caps::MAX_FRAMES_IN_FLIGHT;
		mapped_pointer = nullptr;
	}

	Texture::~Texture() 
	{
		auto allocator = Engine::GetVulkanContext()->GetAllocator();
		vmaDestroyImage(allocator, image, allocation);
	}

} }