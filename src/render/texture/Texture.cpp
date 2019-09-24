#include "Texture.h"
#include "system/Logging.h"
#include "Engine.h"
#include "render/device/VulkanContext.h"
#include "render/device/VulkanUploader.h"
#include "render/buffer/VulkanBuffer.h"

namespace core { namespace Device {

	Texture::Texture(const TextureInitializer& initializer)
	{
		auto allocator = Engine::GetVulkanContext()->GetAllocator();
		auto& device = Engine::GetVulkanDevice();

		if (initializer.mode == TextureInitializer::Mode::RAW_RGB_A) // raw RGB / RGBA, 8 bit per channel
		{
			VkImageCreateInfo create_info = {};
			create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			create_info.imageType = VK_IMAGE_TYPE_2D;
			create_info.extent.width = initializer.width;
			create_info.extent.height = initializer.height;
			create_info.extent.depth = 1;
			create_info.mipLevels = 1;
			create_info.arrayLayers = 1;
			create_info.format = initializer.channel_count == 4 ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_R8G8B8_UNORM;
			create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
			create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			create_info.samples = VK_SAMPLE_COUNT_1_BIT;
			create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			create_info.pNext = nullptr;
			create_info.pQueueFamilyIndices = 0;

			VmaAllocationCreateInfo allocation_create_info = {};
			allocation_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			allocation_create_info.flags = 0;
			VkImage vk_image;
			auto result = vmaCreateImage(allocator, &create_info, &allocation_create_info, &vk_image, &allocation, nullptr);
			if (result == VK_SUCCESS)
			{
				image = vk_image;
			}
			else throw std::runtime_error("can't allocate image");

			vk::DeviceSize size = (vk::DeviceSize)initializer.width * initializer.height * initializer.channel_count;
			auto staging_buffer = std::make_unique<VulkanBuffer>(
				VulkanBufferInitializer(size).SetStaging().Data(initializer.data)
			);

			auto* uploader = Engine::GetVulkanContext()->GetUploader();

			vk::BufferImageCopy region = vk::BufferImageCopy(0, 0, 0)
				.setImageSubresource(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1))
				.setImageOffset(vk::Offset3D({ 0, 0, 0 }))
				.setImageExtent({ initializer.width, initializer.height, 1 });

			std::vector<vk::BufferImageCopy> copies;
			copies.push_back(region);

			auto image_view_info = vk::ImageViewCreateInfo({}, image, vk::ImageViewType::e2D, vk::Format(create_info.format))
				.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
			image_view = device.createImageViewUnique(image_view_info);

			uploader->AddImageToUpload(std::move(staging_buffer), image, 1, 1, std::move(copies));

		}
		else
			throw std::runtime_error("unsupported mode");

	}

	Texture::~Texture() 
	{
		auto allocator = Engine::GetVulkanContext()->GetAllocator();
		vmaDestroyImage(allocator, image, allocation);
	}

} }