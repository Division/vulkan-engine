#include "BitonicSort.h"
#include "render/buffer/GPUBuffer.h"
#include "render/shader/ShaderCache.h"
#include "render/device/VulkanRenderState.h"

namespace render
{
	BitonicSort::BitonicSort(Device::ShaderCache& shader_cache)
	{
		dispatch_args = std::make_unique<Device::GPUBuffer>("bitonic sort indirect commands", 22 * 23 / 2 * sizeof(VkDispatchIndirectCommand), Device::BufferType::Indirect);

		pre_sort_shader = shader_cache.GetShaderProgram(Device::ShaderProgramInfo().AddShader(Device::ShaderProgram::Stage::Compute, L"shaders/bitonic_sort/Bitonic64PreSortCS.hlsl", "main"));
		inner_sort_shader = shader_cache.GetShaderProgram(Device::ShaderProgramInfo().AddShader(Device::ShaderProgram::Stage::Compute, L"shaders/bitonic_sort/Bitonic64InnerSortCS.hlsl", "main"));
		outer_sort_shader = shader_cache.GetShaderProgram(Device::ShaderProgramInfo().AddShader(Device::ShaderProgram::Stage::Compute, L"shaders/bitonic_sort/Bitonic64OuterSortCS.hlsl", "main"));
		indirect_args_shader = shader_cache.GetShaderProgram(Device::ShaderProgramInfo().AddShader(Device::ShaderProgram::Stage::Compute, L"shaders/bitonic_sort/BitonicIndirectArgsCS.hlsl", "main"));
	}

	BitonicSort::~BitonicSort() = default;

	void BitonicSort::Process(
		Device::VulkanRenderState& state,
		const Device::VulkanBuffer& key_index_list,
		const Device::VulkanBuffer& counter_buffer,
		uint32_t counter_offset,
		bool partially_presorted,
		bool ascending
	)
	{
		const uint32_t ElementSizeBytes = sizeof(uvec2);
		const uint32_t MaxNumElements = key_index_list.Size() / ElementSizeBytes;
		const uint32_t AlignedMaxNumElements = NextPowerOfTwo(MaxNumElements);
		const uint32_t MaxIterations = std::log2(std::max(2048u, AlignedMaxNumElements)) - 10;
		const uint32_t NullItem = ascending ? -1 : 0;

		Device::ResourceBindings resources;
		Device::ConstantBindings constants;

		resources.AddBufferBinding("g_IndirectArgsBuffer", dispatch_args->GetBuffer().get(), dispatch_args->GetSize());
		resources.AddBufferBinding("g_SortBuffer", &key_index_list, key_index_list.Size());
		resources.AddBufferBinding("g_CounterBuffer", &counter_buffer, counter_buffer.Size());
		constants.AddUIntBinding(&MaxIterations, "MaxIterations");
		constants.AddUIntBinding(&counter_offset, "CounterOffset");
		constants.AddUIntBinding(&NullItem, "NullItem");
		
		state.Dispatch(*indirect_args_shader, resources, constants, uvec3(1, 1, 1));
		
		const vk::MemoryBarrier compute_read_indirect_barrier = vk::MemoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead);
		state.Barrier({ &compute_read_indirect_barrier, 1 }, vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader);

		assert(partially_presorted); // TODO: implement presort when needed
		/*
		if (!IsPartiallyPreSorted)
		{
			Context.SetPipelineState(ElementSizeBytes == 4 ? s_Bitonic32PreSortCS : s_Bitonic64PreSortCS);
			Context.DispatchIndirect(s_DispatchArgs, 0);
			Context.InsertUAVBarrier(KeyIndexList);
		}
		*/

		uint32_t IndirectArgsOffset = 12;

		// We have already pre-sorted up through k = 2048 when first writing our list, so
		// we continue sorting with k = 4096.  For unnecessarily large values of k, these
		// indirect dispatches will be skipped over with thread counts of 0.

		for (uint32_t k = 4096; k <= AlignedMaxNumElements; k *= 2)
		{
			constants.AddUIntBinding(&k, "k");

			for (uint32_t j = k / 2; j >= 2048; j /= 2)
			{
				constants.AddUIntBinding(&j, "j");
				state.DispatchIndirect(*outer_sort_shader, resources, constants, *dispatch_args->GetBuffer().get(), IndirectArgsOffset);
				const vk::MemoryBarrier compute_read_indirect_barrier = vk::MemoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead);
				state.Barrier({ &compute_read_indirect_barrier, 1 }, vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader);
				IndirectArgsOffset += 12;
			}

			state.DispatchIndirect(*inner_sort_shader, resources, constants, *dispatch_args->GetBuffer().get(), IndirectArgsOffset);
			const vk::MemoryBarrier compute_read_indirect_barrier = vk::MemoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead);
			state.Barrier({ &compute_read_indirect_barrier, 1 }, vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader);
			IndirectArgsOffset += 12;
		}
	}
}