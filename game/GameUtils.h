#pragma once

#include "CommonIncludes.h"

namespace core { namespace Device {

	class VulkanBuffer;

} }

struct Vertex {
	glm::vec2 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}
};

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

void CreateTextureImage(VkImage& out_image, VkDeviceMemory& out_image_memory);

VkImageView CreateTextureImageView(VkDevice device, VkImage image);

void CreateTextureSampler(VkDevice device, VkSampler& out_sampler);

std::unique_ptr<core::Device::VulkanBuffer> CreateVertexBuffer(const std::vector<Vertex>& vertices);
void CreateVertexBuffer(const std::vector<Vertex>& vertices, VkBuffer& out_vertex_buffer, VkDeviceMemory& out_buffer_memory);

void CreateIndexBuffer(const std::vector<uint16_t>& indices, VkBuffer& out_index_buffer, VkDeviceMemory& out_buffer_memory);

void CreateUniformBuffers(uint32_t in_flight_count, std::vector<VkBuffer>& uniformBuffers, std::vector<VkDeviceMemory>& uniformBuffersMemory);

void CreateDescriptorPool(uint32_t in_flight_count, VkDescriptorPool& out_descriptor_pool);

void CreateDescriptorSets(
	uint32_t in_flight_count,
	VkImageView textureImageView,
	VkSampler textureSampler,
	const std::vector<VkBuffer>& uniformBuffers,
	VkDescriptorPool pool,
	VkDescriptorSetLayout layout,
	std::vector<VkDescriptorSet>& descriptorSets
);
