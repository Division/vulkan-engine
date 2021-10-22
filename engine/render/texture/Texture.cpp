#include "Texture.h"
#include "system/Logging.h"
#include "Engine.h"
#include "render/device/Types.h"
#include "render/device/VulkanContext.h"
#include "render/device/VulkanUploader.h"
#include "render/buffer/VulkanBuffer.h"
#include "utils/Math.h"

namespace Device {

	Texture::Texture(const TextureInitializer& initializer)
	{
		auto allocator = Engine::GetVulkanContext()->GetAllocator();
		auto& device = Engine::GetVulkanDevice();

		SetupDefault(initializer);
	}

	Format GetSRGBFormat(Format format, bool srgb)
	{
		switch (format)
		{
			default: return format;
			case Device::Format::R8_unorm: return srgb ? Device::Format::R8_srgb : Device::Format::R8_unorm;
			case Device::Format::R8G8_unorm: return srgb ? Device::Format::R8G8_srgb : Device::Format::R8G8_unorm;
			case Device::Format::R8G8B8_unorm: return srgb ? Device::Format::R8G8B8_srgb : Device::Format::R8G8B8_unorm;
			case Device::Format::R8G8B8A8_unorm: return srgb ? Device::Format::R8G8B8A8_srgb : Device::Format::R8G8B8A8_unorm;
			case Device::Format::BC1_RGB_unorm: return srgb ? Device::Format::BC1_RGB_srgb : Device::Format::BC1_RGB_unorm;
			case Device::Format::BC1_RGBA_unorm: return srgb ? Device::Format::BC1_RGBA_srgb : Device::Format::BC1_RGBA_unorm;
			case Device::Format::BC2_unorm: return srgb ? Device::Format::BC2_srgb : Device::Format::BC2_unorm;
			case Device::Format::BC3_unorm: return srgb ? Device::Format::BC3_srgb : Device::Format::BC3_unorm;
			case Device::Format::BC7_unorm: return srgb ? Device::Format::BC7_srgb : Device::Format::BC7_unorm;
			case Device::Format::Undefined: throw std::runtime_error("Unknown format");
		}
	}

	vk::ImageViewCreateInfo GetImageViewCreateInfo(vk::Image& image, const TextureInitializer& initializer)
	{
		auto image_view_info = vk::ImageViewCreateInfo({}, image, vk::ImageViewType::e2D, vk::Format(GetSRGBFormat(initializer.format, initializer.sRGB)));
		if (initializer.mode == TextureInitializer::DepthBuffer)
			image_view_info.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, initializer.mip_levels, 0, initializer.array_layers));
		else
		{
			image_view_info.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, initializer.mip_levels, 0, initializer.array_layers));
			switch (initializer.num_dimensions)
			{
				case 1:
					image_view_info.setViewType(initializer.is_array ? vk::ImageViewType::e1DArray : vk::ImageViewType::e1D);
					break;
				case 2:
					image_view_info.setViewType(initializer.is_array ? vk::ImageViewType::e2DArray : vk::ImageViewType::e2D);
					break;
				case 3:
					image_view_info.setViewType(vk::ImageViewType::e3D);
					break;
				default:
					assert(false);
			}

			if (initializer.is_cube)
			{
				image_view_info.setViewType(initializer.is_array ? vk::ImageViewType::eCubeArray : vk::ImageViewType::eCube);
			}
				
		}

		image_view_info.setComponents(vk::ComponentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA));

		return image_view_info;
	}

	VkImageCreateInfo GetImageCreateInfo(const TextureInitializer& initializer)
	{
		VkImageCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		create_info.pNext = NULL;
		create_info.imageType = VK_IMAGE_TYPE_2D;
		create_info.format = VkFormat(GetSRGBFormat(initializer.format, initializer.sRGB));
		create_info.extent.width = initializer.width;
		create_info.extent.height = initializer.height;
		create_info.extent.depth = initializer.depth;
		create_info.mipLevels = initializer.mip_levels;
		create_info.arrayLayers = initializer.array_layers;
		create_info.samples = VK_SAMPLE_COUNT_1_BIT;
		create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		create_info.flags = 0;

		switch (initializer.mode)
		{
		case TextureInitializer::Default:
			create_info.usage = (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			break;
		case TextureInitializer::DepthBuffer:
			create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			break;
		case TextureInitializer::ColorTarget:
			create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			break;

		default:
			throw std::runtime_error("unknown mode");
		}

		if (initializer.is_cube)
			create_info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

		if (initializer.force_sampled)
			create_info.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

		create_info.queueFamilyIndexCount = 0;
		create_info.pQueueFamilyIndices = NULL;
		create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

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
		mip_levels = initializer.mip_levels;
		depth = initializer.depth;
		format = GetSRGBFormat(initializer.format, initializer.sRGB);
		size = initializer.data_size;
		if (!size)
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
				staging_buffers[i] = VulkanBuffer::Create(staging_initializer);
			}
		}

		if (initializer.data)
		{
			auto* uploader = Engine::GetVulkanContext()->GetUploader();
			auto upload_staging_buffer = VulkanBuffer::Create(
				VulkanBufferInitializer(size).SetStaging().Data(initializer.data).Name(initializer.name + " Staging")
			);
			uploader->AddImageToUpload(upload_staging_buffer, image, mip_levels, array_layers, initializer.copies.empty() ? GetCopies() : initializer.copies);
		}

		if (!initializer.name.empty())
		{
			Engine::GetVulkanContext()->AssignDebugName((uint64_t)(VkImage)image, vk::DebugReportObjectTypeEXT::eImage, ("[Image] " + initializer.name).c_str());
			Engine::GetVulkanContext()->AssignDebugName((uint64_t)(VkImageView)image_view.get(), vk::DebugReportObjectTypeEXT::eImageView, ("[ImageView] " + initializer.name).c_str());
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
		uploader->AddImageToUpload(staging_buffers[current_staging_buffer], image, 1, array_layers, GetCopies()); // TODO: fix that

		current_staging_buffer = (current_staging_buffer + 1) % caps::MAX_FRAMES_IN_FLIGHT;
		mapped_pointer = nullptr;
	}

	uint32_t Texture::GetHash() const
	{
		return FastHash(&image_view, sizeof(image_view));
	}

	Texture::Handle Texture::Create(const TextureInitializer& initializer)
	{
		return Texture::Handle(std::make_unique<Texture>(initializer));
	}

	Texture::~Texture() 
	{
		auto allocator = Engine::GetVulkanContext()->GetAllocator();
		vmaDestroyImage(allocator, image, allocation);
	}

}