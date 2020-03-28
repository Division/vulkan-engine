#pragma once

#include <stdint.h>
#include "render/device/Types.h"

namespace core::render::synchronization {

	enum OperationBits : int
	{
		Read = 1 << 1,
		Write = 1 << 2,
		GraphicsShader = 1 << 3,
		ComputeShader = 1 >> 4,
		ColorAttachment = 1 << 5,
		DepthStencilAttachment = 1 << 6,
	};

	enum class ResourceOperationType : int
	{
		None = 0,
		GraphicsShaderRead = OperationBits::GraphicsShader | OperationBits::Read,
		GraphicsShaderWrite = OperationBits::GraphicsShader | OperationBits::Write,
		GraphicsShaderReadWrite = OperationBits::GraphicsShader | OperationBits::Write | OperationBits::Read,
		ComputeShaderRead = OperationBits::ComputeShader | OperationBits::Read,
		ComputeShaderWrite = OperationBits::ComputeShader | OperationBits::Write,
		ComputeShaderReadWrite = OperationBits::ComputeShader | OperationBits::Write | OperationBits::Read,
		ColorAttachment = OperationBits::ColorAttachment | OperationBits::Read | OperationBits::Write,
		DepthStencilAttachment = OperationBits::DepthStencilAttachment | OperationBits::Read | OperationBits::Write,
		Present,
	};

	struct ImageAccess
	{
		vk::AccessFlags access = {};
		vk::PipelineStageFlags stage = {};
		vk::ImageLayout layout = vk::ImageLayout::eUndefined;
		uint32_t queue_index = -1;

		bool operator==(const ImageAccess& other)
		{
			return std::tie(access, stage, layout, queue_index) == std::tie(other.access, other.stage, other.layout, other.queue_index);
		}
	};

	struct BufferAccess
	{
		vk::AccessFlags access = {};
		vk::PipelineStageFlags stage = {};
		uint32_t queue_index = -1;

		bool operator==(const BufferAccess& other)
		{
			return std::tie(access, stage, queue_index) == std::tie(other.access, other.stage, other.queue_index);
		}
	};

	ImageAccess GetImageAccessSrc(ResourceOperationType operation_type);
	ImageAccess GetImageAccessDst(ResourceOperationType operation_type);
	BufferAccess GetBufferAccessSrc(ResourceOperationType operation_type);
	BufferAccess GetBufferAccessDst(ResourceOperationType operation_type);

	void ImagePipelineBarrier(
		const ImageAccess src_access, const ImageAccess dst_access, vk::Image image, /*range,*/ vk::CommandBuffer& command_buffer
	);

	void BufferPipelineBarrier(
		const BufferAccess src_access, const BufferAccess dst_access, vk::Buffer buffer, vk::DeviceSize buffer_size, vk::CommandBuffer& command_buffer
	);
}