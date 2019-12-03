#pragma once

#include "CommonIncludes.h"

namespace core { namespace Device {

	enum class Format : int
	{
		Undefined = VK_FORMAT_UNDEFINED,
		R8_unorm = VK_FORMAT_R8_UNORM,
		R8_norm = VK_FORMAT_R8_SNORM,
		R8_uint = VK_FORMAT_R8_UINT,
		R8_int = VK_FORMAT_R8_SINT,
		R8_srgb = VK_FORMAT_R8_SRGB,
		R8G8_unorm = VK_FORMAT_R8G8_UNORM,
		R8G8_norm = VK_FORMAT_R8G8_SNORM,
		R8G8_uint = VK_FORMAT_R8G8_UINT,
		R8G8_int = VK_FORMAT_R8G8_SINT,
		R8G8_srgb = VK_FORMAT_R8G8_SRGB,
		R8G8B8_unorm = VK_FORMAT_R8G8B8_UNORM,
		R8G8B8_norm = VK_FORMAT_R8G8B8_SNORM,
		R8G8B8_uint = VK_FORMAT_R8G8B8_UINT,
		R8G8B8_int = VK_FORMAT_R8G8B8_SINT,
		R8G8B8_srgb = VK_FORMAT_R8G8B8_SRGB,
		R8G8B8A8_unorm = VK_FORMAT_R8G8B8A8_UNORM,
		R8G8B8A8_norm = VK_FORMAT_R8G8B8A8_SNORM,
		R8G8B8A8_uint = VK_FORMAT_R8G8B8A8_UINT,
		R8G8B8A8_int = VK_FORMAT_R8G8B8A8_SINT,
		R8G8B8A8_srgb = VK_FORMAT_R8G8B8A8_SRGB,
		R16_unorm = VK_FORMAT_R16_UNORM,
		R16_norm = VK_FORMAT_R16_SNORM,
		R16_uint = VK_FORMAT_R16_UINT,
		R16_int = VK_FORMAT_R16_SINT,
		R16_float = VK_FORMAT_R16_SFLOAT,
		R16G16_unorm = VK_FORMAT_R16G16_UNORM,
		R16G16_norm = VK_FORMAT_R16G16_SNORM,
		R16G16_uint = VK_FORMAT_R16G16_UINT,
		R16G16_int = VK_FORMAT_R16G16_SINT,
		R16G16_float = VK_FORMAT_R16G16_SFLOAT,
		R16G16B16_unorm = VK_FORMAT_R16G16B16_UNORM,
		R16G16B16_norm = VK_FORMAT_R16G16B16_SNORM,
		R16G16B16_uint = VK_FORMAT_R16G16B16_UINT,
		R16G16B16_int = VK_FORMAT_R16G16B16_SINT,
		R16G16B16_float = VK_FORMAT_R16G16B16_SFLOAT,
		R16G16B16A16_unorm = VK_FORMAT_R16G16B16A16_UNORM,
		R16G16B16A16_norm = VK_FORMAT_R16G16B16A16_SNORM,
		R16G16B16A16_uint = VK_FORMAT_R16G16B16A16_UINT,
		R16G16B16A16_int = VK_FORMAT_R16G16B16A16_SINT,
		R16G16B16A16_float = VK_FORMAT_R16G16B16A16_SFLOAT,
		R32_uint = VK_FORMAT_R32_UINT,
		R32_int = VK_FORMAT_R32_SINT,
		R32_float = VK_FORMAT_R32_SFLOAT,
		R32G32_uint = VK_FORMAT_R32G32_UINT,
		R32G32_int = VK_FORMAT_R32G32_SINT,
		R32G32_float = VK_FORMAT_R32G32_SFLOAT,
		R32G32B32_uint = VK_FORMAT_R32G32B32_UINT,
		R32G32B32_int = VK_FORMAT_R32G32B32_SINT,
		R32G32B32_float = VK_FORMAT_R32G32B32_SFLOAT,
		R32G32B32A32_uint = VK_FORMAT_R32G32B32A32_UINT,
		R32G32B32A32_int = VK_FORMAT_R32G32B32A32_SINT,
		R32G32B32A32_float = VK_FORMAT_R32G32B32A32_SFLOAT,
		D16_unorm = VK_FORMAT_D16_UNORM,
		D32_float = VK_FORMAT_D32_SFLOAT,
		S8_uint = VK_FORMAT_S8_UINT,
		D16_unorm_S8_uint = VK_FORMAT_D16_UNORM_S8_UINT,
		D24_unorm_S8_uint = VK_FORMAT_D24_UNORM_S8_UINT
	};

	enum class AttachmentLoadOp
	{
		Load = VK_ATTACHMENT_LOAD_OP_LOAD,
		Clear = VK_ATTACHMENT_LOAD_OP_CLEAR,
		DontCare = VK_ATTACHMENT_LOAD_OP_DONT_CARE
	};

	enum class AttachmentStoreOp
	{
		Store = VK_ATTACHMENT_STORE_OP_STORE,
		DontCare = VK_ATTACHMENT_STORE_OP_DONT_CARE
	};

	enum class ImageLayout
	{
		Undefined = VK_IMAGE_LAYOUT_UNDEFINED,
		General = VK_IMAGE_LAYOUT_GENERAL,
		ColorAttachmentOptimal = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		DepthStencilAttachmentOptimal = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		DepthStencilReadOnlyOptimal = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
		ShaderReadOnlyOptimal = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		TransferSrcOptimal = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		TransferDstOptimal = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		Preinitialized = VK_IMAGE_LAYOUT_PREINITIALIZED,
		DepthReadOnlyStencilAttachmentOptimal = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL,
		DepthAttachmentStencilReadOnlyOptimal = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL,
		PresentSrc = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		DepthReadOnlyStencilAttachmentOptimalKHR = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL_KHR,
		DepthAttachmentStencilReadOnlyOptimalKHR = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL_KHR
	};

	enum class PipelineBindPoint
	{
		Graphics = VK_PIPELINE_BIND_POINT_GRAPHICS,
		Compute = VK_PIPELINE_BIND_POINT_COMPUTE,
		RayTracingNV = VK_PIPELINE_BIND_POINT_RAY_TRACING_NV
	};

	extern const std::unordered_map<Format, size_t> format_sizes;

} }