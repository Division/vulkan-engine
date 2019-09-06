#include "Engine.h"

#include "render/device/VulkanUtils.h"
#include "render/device/Device.h"
#include "render/device/Device.h"
#include "render/device/VulkanContext.h"
#include "render/buffer/VulkanBuffer.h"

#include "GameUtils.h"

using namespace core;
using namespace core::Device;

void CreateTextureSampler(VkDevice device, VkSampler& out_sampler) {
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	if (vkCreateSampler(device, &samplerInfo, nullptr, &out_sampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}
}

VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format) {
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}

void CreateImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = VulkanUtils::FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(device, image, imageMemory, 0);
}

void CreateTextureImage(VkImage& out_image, VkDeviceMemory& out_image_memory) {
	auto* engine = Engine::Get();
	auto* device = engine->GetDevice();
	auto* context = device->GetContext();
	auto vk_device = context->GetDevice();
	auto vk_physical_device = context->GetPhysicalDevice();

	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load("textures/depth_correct.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	VulkanUtils::CreateBuffer(vk_device, vk_physical_device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(vk_device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(vk_device, stagingBufferMemory);

	stbi_image_free(pixels);

	CreateImage(vk_device, vk_physical_device, texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, out_image, out_image_memory);

	VulkanUtils::TransitionImageLayout(*context, out_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	VulkanUtils::CopyBufferToImage(*context, stagingBuffer, out_image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	VulkanUtils::TransitionImageLayout(*context, out_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(vk_device, stagingBuffer, nullptr);
	vkFreeMemory(vk_device, stagingBufferMemory, nullptr);
}

VkImageView CreateTextureImageView(VkDevice device, VkImage image) {
	return CreateImageView(device, image, VK_FORMAT_R8G8B8A8_UNORM);
}

std::unique_ptr<VulkanBuffer> CreateVertexBuffer(const std::vector<Vertex>& vertices) {
	auto* engine = Engine::Get();
	auto* device = engine->GetDevice();
	auto* context = device->GetContext();
	auto vk_device = context->GetDevice();
	auto vk_physical_device = context->GetPhysicalDevice();

	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	auto initializer = VulkanBufferInitializer(bufferSize)
		.Usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
		.MemoryUsage(VMA_MEMORY_USAGE_GPU_ONLY)
		.Data((void*)vertices.data());
	return std::make_unique<VulkanBuffer>(initializer);
}

void CreateVertexBuffer(const std::vector<Vertex>& vertices, VkBuffer& out_vertex_buffer, VkDeviceMemory& out_buffer_memory) {
	auto* engine = Engine::Get();
	auto* device = engine->GetDevice();
	auto* context = device->GetContext();
	auto vk_device = context->GetDevice();
	auto vk_physical_device = context->GetPhysicalDevice();

	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	VulkanUtils::CreateBuffer(vk_device, vk_physical_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(vk_device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(vk_device, stagingBufferMemory);

	VulkanUtils::CreateBuffer(vk_device, vk_physical_device, bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		out_vertex_buffer, out_buffer_memory
	);

	VulkanUtils::CopyBuffer(*context, stagingBuffer, out_vertex_buffer, bufferSize);

	vkDestroyBuffer(vk_device, stagingBuffer, nullptr);
	vkFreeMemory(vk_device, stagingBufferMemory, nullptr);
}

void CreateIndexBuffer(const std::vector<uint16_t>& indices, VkBuffer& out_index_buffer, VkDeviceMemory& out_buffer_memory) {
	auto* engine = Engine::Get();
	auto* device = engine->GetDevice();
	auto* context = device->GetContext();
	auto vk_device = context->GetDevice();
	auto vk_physical_device = context->GetPhysicalDevice();

	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	VulkanUtils::CreateBuffer(vk_device, vk_physical_device, bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		stagingBuffer, stagingBufferMemory
	);

	void* data;
	vkMapMemory(vk_device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(vk_device, stagingBufferMemory);

	VulkanUtils::CreateBuffer(vk_device, vk_physical_device, bufferSize, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		out_index_buffer, out_buffer_memory
	);

	VulkanUtils::CopyBuffer(*context, stagingBuffer, out_index_buffer, bufferSize);

	vkDestroyBuffer(vk_device, stagingBuffer, nullptr);
	vkFreeMemory(vk_device, stagingBufferMemory, nullptr);
}

void CreateUniformBuffers(uint32_t in_flight_count, std::vector<VkBuffer>& uniformBuffers, std::vector<VkDeviceMemory>& uniformBuffersMemory) {
	auto* engine = Engine::Get();
	auto* device = engine->GetDevice();
	auto* context = device->GetContext();
	auto vk_device = context->GetDevice();
	auto vk_physical_device = context->GetPhysicalDevice();
	
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);

	uniformBuffers.resize(in_flight_count);
	uniformBuffersMemory.resize(in_flight_count);

	for (size_t i = 0; i < in_flight_count; i++) {
		VulkanUtils::CreateBuffer(vk_device, vk_physical_device, bufferSize, 
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		uniformBuffers[i], uniformBuffersMemory[i]);
	}
}

void CreateDescriptorPool(uint32_t in_flight_count, VkDescriptorPool& out_descriptor_pool) {
	auto* engine = Engine::Get();
	auto* device = engine->GetDevice();
	auto* context = device->GetContext();
	auto vk_device = context->GetDevice();
	auto vk_physical_device = context->GetPhysicalDevice();

	std::array<VkDescriptorPoolSize, 2> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = in_flight_count;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = in_flight_count;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = in_flight_count;

	if (vkCreateDescriptorPool(vk_device, &poolInfo, nullptr, &out_descriptor_pool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void CreateDescriptorSets(
	uint32_t in_flight_count,
	VkImageView textureImageView,
	VkSampler textureSampler,
	const std::vector<VkBuffer>& uniformBuffers, 
	VkDescriptorPool pool, 
	VkDescriptorSetLayout layout, 
	std::vector<VkDescriptorSet>& descriptorSets
) {
	auto* engine = Engine::Get();
	auto* device = engine->GetDevice();
	auto* context = device->GetContext();
	auto vk_device = context->GetDevice();
	auto vk_physical_device = context->GetPhysicalDevice();

	std::vector<VkDescriptorSetLayout> layouts(in_flight_count, layout);
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = pool;
	allocInfo.descriptorSetCount = in_flight_count;
	allocInfo.pSetLayouts = layouts.data();

	descriptorSets.resize(in_flight_count);
	if (vkAllocateDescriptorSets(vk_device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < in_flight_count; i++) {
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = textureImageView;
		imageInfo.sampler = textureSampler;

		std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(vk_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}