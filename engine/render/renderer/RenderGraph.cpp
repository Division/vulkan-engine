#include "RenderGraph.h"
#include "utils/DataStructures.h"
#include "render/device/VulkanRenderPass.h"
#include "render/buffer/VulkanBuffer.h"
#include "render/device/VulkanRenderTarget.h"
#include "render/device/VulkanSwapchain.h"
#include "render/device/VulkanRenderState.h"
#include "render/device/VulkanContext.h"
#include "render/device/VkObjects.h"
#include "render/debug/Profiler.h"
#include "render/texture/Texture.h"
#include "lib/optick/src/optick.h"

#include "Engine.h"

#define DEBUG_LOG false

using namespace Device;

namespace render { namespace graph {

	using namespace synchronization;

	uint32_t GetOperationIndex(DependencyNode& node, uint32_t pass_index, bool is_output);
	void AddOutputOperation(DependencyNode& node, Pass& pass);
	void AddInputOperation(DependencyNode& node, Pass& pass, InputUsage usage);

	DependencyNode* DependencyNode::PresentSwapchain()
	{
		assert(resource->type == ResourceType::Attachment);
		assert(resource->GetAttachment()->IsSwapchain());
		present_swapchain = true;
		return this;
	}

	bool RenderGraph::ResourceRegistered(void* resource)
	{
		return std::any_of(resources.begin(), resources.end(), [=](auto& item) { return item->resource_pointer == resource; });
	}

	ResourceWrapper* RenderGraph::RegisterAttachment(VulkanRenderTargetAttachment& attachment)
	{
		assert(!ResourceRegistered(&attachment));
		resources.emplace_back(std::make_unique<ResourceWrapper>(ResourceType::Attachment, &attachment));
		return resources.back().get();
	}

	ResourceWrapper* RenderGraph::RegisterBuffer(VulkanBuffer& buffer)
	{
		assert(!ResourceRegistered(&buffer));
		resources.emplace_back(std::make_unique<ResourceWrapper>(ResourceType::Buffer, &buffer));
		return resources.back().get();
	}

	void RenderGraph::Clear()
	{
		render_passes.clear();
		resources.clear();
		nodes.clear();
	}

	void RenderGraph::ClearCache()
	{
		render_pass_cache.clear();
		render_target_cache.clear();
	}

	void RenderGraph::AddInput(DependencyNode& node, InputUsage usage)
	{
		current_render_pass->input_nodes.push_back(std::make_pair(&node, usage));
	}

	DependencyNode* RenderGraph::AddOutput(ResourceWrapper& resource)
	{
		nodes.emplace_back(std::make_unique<DependencyNode>());
		auto* node = nodes.back().get();
		node->index = nodes.size() - 1;
		node->resource = &resource;
		node->render_pass = current_render_pass;
		current_render_pass->output_nodes.push_back(node);

		return node;
	}

	void RenderGraph::SetCompute(bool is_async)
	{
		assert(!is_async); // async not supported yet
		current_render_pass->is_compute = true;
		current_render_pass->is_async = is_async;
	}

	void RenderGraph::Prepare()
	{
		auto* context = Engine::GetVulkanContext();

		for (uint32_t i = 0; i < render_passes.size(); i++)
		{
			auto* pass = render_passes[i].get();
			pass->index = i;
			pass->queue_family_index = context->GetQueueFamilyIndex((pass->is_compute && pass->is_async) ? PipelineBindPoint::Compute : PipelineBindPoint::Graphics);
			for (auto* output_node : pass->output_nodes)
			{
				AddOutputOperation(*output_node, *pass);
			}

			for (auto& data : pass->input_nodes)
			{
				AddInputOperation(*data.first, *pass, data.second);
			}
		}

		// Checking ownership transfers
		for (uint32_t i = 0; i < render_passes.size(); i++)
		{
			auto* pass = render_passes[i].get();

			uint32_t smallest_pass_index = -1; // First pass after current one that needs ownership transfer for any output of the current one

			for (auto* output_node : pass->output_nodes)
			{
				auto& operations = output_node->resource->operations;
				auto operation_index = GetOperationIndex(*output_node, pass->index, true);
				uint32_t index = operation_index + 1;
				if (index < operations.size())
				{
					if (operations[index].queue_family_index != pass->queue_family_index)
					{
						smallest_pass_index = std::min(operations[index].pass_index, smallest_pass_index);
						pass->signal_stages |= GetImageAccessDst(operations[index].type).stage;
						output_node->should_transfer_ownership = true;
					}
				}
			}

			for (auto& input_node_pair : pass->input_nodes)
			{
				auto* input_node = input_node_pair.first;
				auto& operations = input_node->resource->operations;
				auto operation_index = GetOperationIndex(*input_node, pass->index, false);
				uint32_t index = operation_index + 1;
				if (index < operations.size())
				{
					if (operations[index].queue_family_index != pass->queue_family_index)
					{
						smallest_pass_index = std::min(operations[index].pass_index, smallest_pass_index);
						pass->signal_stages |= GetImageAccessDst(operations[index].type).stage;
						input_node->should_transfer_ownership = true;
					}
				}
			}

			// pass signals if it has pending ownership transfer (e.g. following pass uses current pass output in a different queue)
			if (smallest_pass_index != -1)
			{
				pass->signal_semaphore = GetSemaphore();
				render_passes[smallest_pass_index]->wait_semaphores.semaphores.push_back(pass->signal_semaphore);
				render_passes[smallest_pass_index]->wait_semaphores.stage_flags.push_back(pass->signal_stages);
			}
		}
	}

	void AddOutputOperation(DependencyNode& node, Pass& pass)
	{
		auto* resource = node.resource;
		ResourceOperation operation;
		operation.pass_index = pass.index;

		if (resource->type == ResourceType::Attachment)
		{
			bool is_color = resource->GetAttachment()->GetType() == VulkanRenderTargetAttachment::Type::Color;
			operation.type = is_color ? ResourceOperationType::ColorAttachment : ResourceOperationType::DepthStencilAttachment;
		}
		else
		{
			operation.type = pass.is_compute ? ResourceOperationType::ComputeShaderReadWrite : ResourceOperationType::GraphicsShaderReadWrite;
		}

		operation.queue_family_index = pass.queue_family_index;

		node.pass_operation_index = resource->operations.size();
		resource->operations.push_back(operation);

		if (node.present_swapchain)
		{
			ResourceOperation present_operation;
			present_operation.pass_index = pass.index + 1;
			present_operation.type = ResourceOperationType::Present;
			present_operation.queue_family_index = pass.queue_family_index;
			resource->operations.push_back(present_operation);
		}
	}

	void AddInputOperation(DependencyNode& node, Pass& pass, InputUsage usage)
	{
		auto* resource = node.resource;
		ResourceOperation operation;
		operation.pass_index = pass.index;
		operation.queue_family_index = pass.queue_family_index;
		operation.type = pass.is_compute ? ResourceOperationType::ComputeShaderRead : ResourceOperationType::GraphicsShaderRead;

		if (resource->type == ResourceType::Attachment)
		{
			if (usage == InputUsage::DepthAttachment)
				operation.type = ResourceOperationType::DepthStencilAttachment; // TODO: DepthStencilAttachmentRead
		}

		resource->operations.push_back(operation);
	}

	std::tuple<VulkanRenderPassInitializer, VulkanRenderTargetInitializer, vec2> GetPassInitializer(Pass* pass)
	{
		VulkanRenderPassInitializer render_pass_initializer;
		VulkanRenderTargetInitializer render_target_initializer(0,0);
		vec2 size(0);

		if (pass->is_compute)
		{
			throw std::runtime_error("Error: compute doesn't have render pass");
		} else
		{
			for (auto* output : pass->output_nodes)
			{
				auto* resource = output->resource;
				auto operation_index = GetOperationIndex(*output, pass->index, true);
				auto& operation = resource->operations[operation_index];
				auto* prev_operation = operation_index > 0 ? &resource->operations[operation_index - 1] : nullptr;

				switch (resource->type)
				{
				case ResourceType::Attachment:
					{
						auto& attachment = *resource->GetAttachment();
						render_pass_initializer.AddAttachment(attachment);
						auto initial_layout = vk::ImageLayout::eUndefined;

						AttachmentLoadOp load_op;
						if (prev_operation)
						{
							load_op = output->should_clear ? AttachmentLoadOp::Clear : AttachmentLoadOp::Load;
							if (!output->should_clear)
								initial_layout = GetImageAccessDst(prev_operation->type).layout;
						}
						else
							load_op = output->should_clear ? AttachmentLoadOp::Clear : AttachmentLoadOp::DontCare;
							
						render_pass_initializer.SetLoadStoreOp(load_op, AttachmentStoreOp::Store);

						render_target_initializer.Size(attachment.GetWidth(), attachment.GetHeight());
						size = glm::max(size, vec2(attachment.GetWidth(), attachment.GetHeight()));
						render_target_initializer.AddAttachment(attachment);


						if (attachment.GetType() == VulkanRenderTargetAttachment::Type::Color)
						{
							auto final_layout = attachment.IsSwapchain() ? ImageLayout::PresentSrc : ImageLayout::ColorAttachmentOptimal;
							render_pass_initializer.SetImageLayout((ImageLayout)initial_layout, ImageLayout::ColorAttachmentOptimal);
						}
						else
							render_pass_initializer.SetImageLayout((ImageLayout)initial_layout, ImageLayout::DepthStencilAttachmentOptimal);

						break;
					}

				default:
					throw std::runtime_error("not implemented");
				}
			}

			for (auto& input : pass->input_nodes)
			{
				auto* resource = input.first->resource;
				auto usage = input.second;

				// Using depth buffer from previous depth-only pass
				if (usage == InputUsage::DepthAttachment)
				{
					assert(!render_pass_initializer.has_depth);
					assert(resource->type == ResourceType::Attachment && resource->GetAttachment()->GetType() == VulkanRenderTargetAttachment::Type::Depth);
					auto& attachment = *resource->GetAttachment();
					size = glm::max(size, vec2(attachment.GetWidth(), attachment.GetHeight()));

					auto operation_index = GetOperationIndex(*input.first, pass->index, false);
					auto& operation = input.first->resource->operations[operation_index];
					auto layout = GetImageAccessDst(operation.type).layout;

					render_pass_initializer.AddAttachment(attachment);
					render_pass_initializer.SetLoadStoreOp(AttachmentLoadOp::Load, AttachmentStoreOp::DontCare);
					render_pass_initializer.SetImageLayout((ImageLayout)layout, ImageLayout::DepthStencilReadOnlyOptimal);

					render_target_initializer.Size(attachment.GetWidth(), attachment.GetHeight());
					render_target_initializer.AddAttachment(attachment);
				}
			}
		}

		return std::make_tuple(std::move(render_pass_initializer), std::move(render_target_initializer), size);
	}

	VulkanRenderPass* RenderGraph::GetRenderPass(const VulkanRenderPassInitializer& initializer)
	{
		auto hash = initializer.GetHash();
		auto iter = render_pass_cache.find(hash);
		if (iter != render_pass_cache.end())
			return iter->second.get();
		
		auto render_pass = std::make_unique<VulkanRenderPass>(initializer);
		auto* result = render_pass.get();
		render_pass_cache[hash] = std::move(render_pass);

		return result;
	}

	VulkanRenderTarget* RenderGraph::GetRenderTarget(const VulkanRenderTargetInitializer& initializer)
	{
		auto hash = initializer.GetHash();
		auto iter = render_target_cache.find(hash);
		if (iter != render_target_cache.end())
			return iter->second.get();

		auto render_target = std::make_unique<VulkanRenderTarget>(initializer);
		auto* result = render_target.get();
		render_target_cache[hash] = std::move(render_target);

		return result;
	}

	void PipelineBarrier(uint32_t operation_index, DependencyNode& node, vk::CommandBuffer& command_buffer)
	{
		auto* resource = node.resource;
		auto& current_operation = resource->operations[operation_index];
		auto& prev_operation = operation_index > 0 ? resource->operations[operation_index - 1] : current_operation;
		bool is_buffer = node.resource->type == ResourceType::Buffer;

		if (is_buffer)
		{
			auto src_access = GetBufferAccessSrc(prev_operation.type);
			if (operation_index == 0)
			{
				src_access.access = {};
			}

			auto dst_access = GetBufferAccessDst(current_operation.type);
			//if (src_access == dst_access)
				//return;

			BufferPipelineBarrier(src_access, dst_access, resource->GetBuffer()->Buffer(), resource->GetBuffer()->Size(), command_buffer);
		}
		else
		{

			auto src_access = GetImageAccessSrc(prev_operation.type);
			if (operation_index == 0)
			{
				src_access.access = {};
				src_access.layout = vk::ImageLayout::eUndefined;
			}
			auto dst_access = GetImageAccessDst(current_operation.type);
			//if (src_access == dst_access)
				//return;

			ImagePipelineBarrier(src_access, dst_access, resource->GetAttachment()->GetImage(), command_buffer);
		}
	}

	uint32_t GetOperationIndex(DependencyNode& node, uint32_t pass_index, bool is_output)
	{
		auto& resource = node.resource;
		uint32_t operation_index = -1;
		if (is_output)
			operation_index = node.pass_operation_index;
		else
		{
			for (uint32_t i = 0; i < resource->operations.size(); i++)
				if (resource->operations[i].pass_index == pass_index)
				{
					operation_index = i;
					break;
				}
		}
		if (operation_index == -1)
			throw std::runtime_error("logic_error");

		return operation_index;
	}

	void RenderGraph::ApplyPreBarriers(Pass& pass, VulkanRenderState& state)
	{
		auto command_buffer = state.GetCurrentCommandBuffer()->GetCommandBuffer();

		for (auto* node : pass.output_nodes)
		{
			auto operation_index = GetOperationIndex(*node, pass.index, true);
			PipelineBarrier(operation_index, *node, command_buffer);
		}

		for (auto node_data : pass.input_nodes)
		{
			auto operation_index = GetOperationIndex(*node_data.first, pass.index, false);
			PipelineBarrier(operation_index, *node_data.first, command_buffer);
		}
	}

	void RenderGraph::ApplyPostBarriers(Pass& pass, VulkanRenderState& state)
	{
		auto command_buffer = state.GetCurrentCommandBuffer()->GetCommandBuffer();
		for (auto* node : pass.output_nodes)
		{
			if (node->should_transfer_ownership)
			{
				auto operation_index = GetOperationIndex(*node, pass.index, true) + 1;
				PipelineBarrier(operation_index, *node, command_buffer);
			}

			if (node->present_swapchain)
			{
				auto* attachment = node->resource->GetAttachment();
				auto operation_index = GetOperationIndex(*node, pass.index + 1, false); // false here to perform full search
				PipelineBarrier(operation_index, *node, command_buffer);
			}
		}
	}

	void SetResourceWriteSemaphore(Pass* pass, vk::Semaphore semaphore)
	{
		for (auto* output : pass->output_nodes)
			output->resource->semaphore = semaphore;
	}


	void RenderGraph::RecordPass(Pass* pass)
	{
		//OPTICK_EVENT_DYNAMIC(pass->name.c_str());
		//bool is_graphics = !pass->is_compute;
		//PipelineBindPoint binding_point = is_graphics ? PipelineBindPoint::Graphics : PipelineBindPoint::Compute;

		//auto* context = Engine::GetVulkanContext();
		//auto* state = context->GetRenderState();
		//state->BeginRecording(binding_point);
		//ApplyPreBarriers(*pass, *state);
		//auto* command_buffer = state->GetCurrentCommandBuffer();

		//context->BeginDebugMarker(*command_buffer, pass->name.c_str());
		//profiler::StartMeasurement(*command_buffer, pass->index, pass->profiler_name);

		//if (is_graphics)
		//{
		//	auto initializer = GetPassInitializer(pass);
		//	auto* vulkan_pass = GetRenderPass(std::get<0>(initializer));
		//	auto* vulkan_render_target = GetRenderTarget(std::get<1>(initializer));
		//	auto viewport = vec4(0, 0, std::get<2>(initializer));

		//	uint32_t attach_index = 0;
		//	for (auto* output : pass->output_nodes)
		//	{
		//		if (output->resource->type == ResourceType::Attachment)
		//			state->SetClearValue(attach_index++, output->clear_value);
		//	}

		//	state->SetScissor(viewport);
		//	state->SetViewport(viewport);

		//	state->BeginRendering(*vulkan_render_target, *vulkan_pass);
		//}

		//pass->record_callback(*state);

		//if (is_graphics)
		//	state->EndRendering();

		//ApplyPostBarriers(*pass, *state);
		//
		//profiler::FinishMeasurement(*command_buffer, pass->index, pass->profiler_name);
		//context->EndDebugMarker(*command_buffer);
		//state->EndRecording();

		////FrameCommandBufferData data(
		////	command_buffer->GetCommandBuffer(),
		////	pass->signal_semaphore,
		////	std::move(pass->wait_semaphores),
		////	binding_point
		////);

		////data.command_buffer = command_buffer->GetCommandBuffer();
		////data.queue = binding_point;
		////context->AddFrameCommandBuffer(data);
	}

	void RenderGraph::RecordCommandBuffers()
	{
		OPTICK_EVENT();
		for (auto& pass : render_passes)
		{
			RecordPass(pass.get());
		}
	}

	void RenderGraph::Render()
	{
		UpdateInFlightSemaphores();
		RecordCommandBuffers();
	}

	vk::Semaphore RenderGraph::GetSemaphore()
	{
		std::scoped_lock lock(semaphore_mutex);
		vk::Semaphore semaphore;

		if (available_semaphores.size())
		{
			semaphore = available_semaphores.back();
			available_semaphores.pop_back();
		} else
		{
			auto device = Engine::GetVulkanDevice();
			semaphores.push_back(device.createSemaphoreUnique(vk::SemaphoreCreateInfo()));
			semaphore = semaphores.back().get();
		}

		in_flight_semaphores.push_back(std::make_pair(0, semaphore));
		return semaphore;
	}

	void RenderGraph::UpdateInFlightSemaphores()
	{
		std::scoped_lock lock(semaphore_mutex);

		for (auto& pair : in_flight_semaphores)
		{
			pair.first += 1;
		}

		const int in_flight_count = 3;
		auto remove_pos = std::remove_if(in_flight_semaphores.begin(), in_flight_semaphores.end(), [=](auto& pair) { return pair.first >= in_flight_count; });
		for (auto iter = remove_pos; iter != in_flight_semaphores.end(); iter++)
		{
			available_semaphores.push_back(iter->second);
		}

		in_flight_semaphores.erase(remove_pos, in_flight_semaphores.end());
	}

} }