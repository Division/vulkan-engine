#include "RenderGraph.h"
#include "utils/DataStructures.h"
#include "render/device/VulkanRenderPass.h"
#include "render/device/VulkanRenderTarget.h"
#include "render/device/VulkanContext.h"
#include "render/device/VulkanSwapchain.h"
#include "render/device/VulkanRenderState.h"
#include "render/device/VkObjects.h"
#include "render/texture/Texture.h"
#include "lib/optick/src/optick.h"
#include "Engine.h"

#define DEBUG_LOG false

namespace core { namespace render { namespace graph {

	uint32_t GetOperationIndex(DependencyNode& node, uint32_t pass_index, bool is_output);

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

	void RenderGraph::SetCompute()
	{
		current_render_pass->is_compute = true;
	}

	void RenderGraph::Prepare()
	{
		utils::Graph graph(nodes.size());
		for (auto& pass : render_passes)
			for (auto& input : pass->input_nodes)
				for (auto& output : pass->output_nodes)
					graph.AddEdge(input.first->index, output->index);

		std::stack<uint32_t> stack;
		std::stack<uint32_t> execution_order;

		std::for_each(nodes.begin(), nodes.end(), [](auto& node) {
			node->on_stack = false;
			node->visited = false;
		});

		for (int i = 0; i < nodes.size(); i++)
		{
			if (nodes[i]->visited)
				continue;

			stack.push(i);

			while (!stack.empty())
			{
				uint32_t vertex = stack.top();

				if (!nodes[vertex]->visited)
				{
					nodes[vertex]->on_stack = true;
					nodes[vertex]->visited = true;
				} else
				{
					nodes[vertex]->on_stack = false;
					execution_order.push(vertex);
					stack.pop();
				}

				auto& bucket = graph.GetVertexBucket(vertex);
				for (uint32_t connected_vertex : bucket)
				{
					if (!nodes[connected_vertex]->visited)
						stack.push(connected_vertex);
					else if (nodes[connected_vertex]->on_stack)
						throw std::runtime_error("not a DAG");
				}
			}
		}

		int current_order = 0;
		while (!execution_order.empty())
		{
			nodes[execution_order.top()]->order = current_order;
			nodes[execution_order.top()]->render_pass->order = std::max(nodes[execution_order.top()]->render_pass->order, current_order);

			current_order += 1;
			execution_order.pop();
		}

		std::sort(render_passes.begin(), render_passes.end(), [](auto& a, auto& b) {
			return a->order < b->order;
		});

		PrepareResourceOperations();
#if DEBUG_LOG
		/*for (auto& pass : render_passes)
		{
			if (pass->order == -1)
				continue;

			OutputDebugStringA("Pass ");
			OutputDebugStringA(pass->name);
			OutputDebugStringA("\n");
		}

		OutputDebugStringA("=============================\n"); */
#endif

	}

	void AddOutputOperation(DependencyNode& node, Pass& pass)
	{
		auto* resource = node.resource;
		ResourceOperation operation;
		operation.operation = OperationType::Write;
		operation.pass_index = pass.index;

		if (resource->type == ResourceType::Attachment)
		{
			bool is_color = resource->GetAttachment()->GetType() == VulkanRenderTargetAttachment::Type::Color;
			operation.access |= is_color ? vk::AccessFlagBits::eColorAttachmentWrite : vk::AccessFlagBits::eDepthStencilAttachmentWrite;
			operation.stage |= is_color ? vk::PipelineStageFlagBits::eColorAttachmentOutput : vk::PipelineStageFlagBits::eLateFragmentTests;
			operation.layout = is_color ? ImageLayout::ColorAttachmentOptimal : ImageLayout::DepthStencilAttachmentOptimal;
		}
		else
		{
			throw std::runtime_error("unsupported");
		}

		node.pass_operation_index = resource->operations.size();
		resource->operations.push_back(operation);

		if (node.present_swapchain)
		{
			ResourceOperation present_operation;
			present_operation.operation = OperationType::Read;
			present_operation.pass_index = pass.index + 1;
			present_operation.access |= vk::AccessFlagBits::eColorAttachmentRead;
			present_operation.stage |= vk::PipelineStageFlagBits::eColorAttachmentOutput;
			present_operation.layout = ImageLayout::PresentSrc;
			resource->operations.push_back(present_operation);
		}
	}

	void AddInputOperation(DependencyNode& node, Pass& pass, InputUsage usage)
	{
		auto* resource = node.resource;
		ResourceOperation operation;
		operation.operation = OperationType::Read;
		operation.pass_index = pass.index;

		if (resource->type == ResourceType::Attachment)
		{
			bool is_color = resource->GetAttachment()->GetType() == VulkanRenderTargetAttachment::Type::Color;

			operation.access |= is_color ? vk::AccessFlagBits::eShaderRead : vk::AccessFlagBits::eDepthStencilAttachmentRead;
			operation.stage |= is_color ? vk::PipelineStageFlagBits::eFragmentShader : vk::PipelineStageFlagBits::eLateFragmentTests;
			operation.layout = usage == InputUsage::DepthAttachment ? ImageLayout::DepthStencilReadOnlyOptimal : ImageLayout::ShaderReadOnlyOptimal;
		}
		else
		{
			throw std::runtime_error("unsupported");
		}

		resource->operations.push_back(operation);
	}

	void RenderGraph::PrepareResourceOperations()
	{
		for (uint32_t i = 0; i < render_passes.size(); i++)
		{
			auto* pass = render_passes[i].get();
			pass->index = i;
			for (auto* output_node : pass->output_nodes)
			{
				AddOutputOperation(*output_node, *pass);
			}

			for (auto& data : pass->input_nodes)
			{
				AddInputOperation(*data.first, *pass, data.second);
			}
		}
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
						auto initial_layout = ImageLayout::Undefined;

						AttachmentLoadOp load_op;
						if (prev_operation)
						{
							load_op = output->should_clear ? AttachmentLoadOp::Clear : AttachmentLoadOp::Load;
							initial_layout = prev_operation->layout;
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
							render_pass_initializer.SetImageLayout(initial_layout, ImageLayout::ColorAttachmentOptimal);
						}
						else
							render_pass_initializer.SetImageLayout(initial_layout, ImageLayout::DepthStencilAttachmentOptimal);

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

					render_pass_initializer.AddAttachment(attachment);
					render_pass_initializer.SetLoadStoreOp(AttachmentLoadOp::Load, AttachmentStoreOp::DontCare);
					render_pass_initializer.SetImageLayout(operation.layout, ImageLayout::DepthStencilReadOnlyOptimal);

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

	struct ImageBarrierData
	{
		vk::ImageAspectFlags image_aspect_flags = {};
		vk::ImageLayout src_layout = vk::ImageLayout::eUndefined;
		vk::ImageLayout dst_layout = vk::ImageLayout::eUndefined;
		vk::AccessFlags src_access_flags = {};
		vk::AccessFlags dst_access_flags = {};
		vk::PipelineStageFlags src_stage = {};
		vk::PipelineStageFlags dst_stage = {};
		vk::Image image = nullptr;
	};

	void ImagePipelineBarrier(
		ImageBarrierData& data,
		vk::CommandBuffer& command_buffer
	) {
		auto* context = Engine::GetVulkanContext();

		vk::ImageSubresourceRange range(
			data.image_aspect_flags,
			0, 1, 0, 1
		);

		vk::ImageMemoryBarrier image_memory_barrier(
			data.src_access_flags, data.dst_access_flags,
			data.src_layout, data.dst_layout, 
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, 
			data.image, range
		);

		command_buffer.pipelineBarrier(
			data.src_stage, 
			data.dst_stage,
			{}, 
			0, nullptr, 
			0, nullptr, 
			1, &image_memory_barrier
		);
	}

	ImageBarrierData GetImageBarrierData(uint32_t operation_index, DependencyNode& node)
	{
		assert(node.resource->type == ResourceType::Attachment);
		auto& resource = node.resource;
		bool is_color = resource->GetAttachment()->GetType() == VulkanRenderTargetAttachment::Type::Color;
		ImageBarrierData data;
		
		// Has previous pass
		if (operation_index > 0)
		{
			auto& prev_operation = resource->operations[operation_index - 1];
			data.src_access_flags = prev_operation.access;
			data.src_stage = prev_operation.stage;
			data.src_layout = (vk::ImageLayout)prev_operation.layout;
		}
		else
		{
			data.src_stage = is_color ? vk::PipelineStageFlagBits::eColorAttachmentOutput : vk::PipelineStageFlagBits::eLateFragmentTests;
		}

		auto& current_operation = resource->operations[operation_index];
		data.dst_access_flags = current_operation.access;
		data.dst_stage = current_operation.stage;
		data.dst_layout = (vk::ImageLayout)current_operation.layout;
		data.image = resource->GetAttachment()->GetImage();
		data.image_aspect_flags = is_color
			? vk::ImageAspectFlagBits::eColor
			: vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;

		return data;
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
			ImagePipelineBarrier(GetImageBarrierData(operation_index, *node), command_buffer);
		}

		for (auto node_data : pass.input_nodes)
		{
			auto operation_index = GetOperationIndex(*node_data.first, pass.index, false);
			ImagePipelineBarrier(GetImageBarrierData(operation_index, *node_data.first), command_buffer);
		}
	}

	void RenderGraph::ApplyPostBarriers(Pass& pass, VulkanRenderState& state)
	{
		auto command_buffer = state.GetCurrentCommandBuffer()->GetCommandBuffer();
		for (auto* node : pass.output_nodes)
		{
			ImageLayout target_layout = ImageLayout::Undefined;

			switch (node->resource->type)
			{
			case ResourceType::Attachment:
			{
				auto* attachment = node->resource->GetAttachment();
				if (!attachment->IsSwapchain() || !node->present_swapchain)
					return;

				auto operation_index = GetOperationIndex(*node, pass.index + 1, false);
				ImagePipelineBarrier(GetImageBarrierData(operation_index, *node), command_buffer);
				break;
			}

			default:
				throw std::runtime_error("not implemented");
			}

		}
	}

	void SetResourceWriteSemaphore(Pass* pass, vk::Semaphore semaphore)
	{
		for (auto* output : pass->output_nodes)
			output->resource->semaphore = semaphore;
	}


	void RenderGraph::RecordGraphicsPass(Pass* pass)
	{
		/*OutputDebugStringA("Recording Pass ");
		OutputDebugStringA(pass->name);
		OutputDebugStringA("\n"); */
		OPTICK_EVENT(pass->name);

		auto* context = Engine::GetVulkanContext();
		auto* state = context->GetRenderState();
		auto initializer = GetPassInitializer(pass);
		auto* vulkan_pass = GetRenderPass(std::get<0>(initializer));
		auto* vulkan_render_target = GetRenderTarget(std::get<1>(initializer));
		auto viewport = vec4(0, 0, std::get<2>(initializer));

		uint32_t attach_index = 0;
		for (auto* output : pass->output_nodes)
		{
			if (output->resource->type == ResourceType::Attachment)
				state->SetClearValue(attach_index++, output->clear_value);
		}

		auto* command_buffer = state->GetCurrentCommandBuffer();
		state->BeginRecording();
		ApplyPreBarriers(*pass, *state);

		state->SetScissor(viewport);
		state->SetViewport(viewport);

		state->BeginRendering(*vulkan_render_target, *vulkan_pass);
		pass->record_callback(*state);
		state->EndRendering();

		ApplyPostBarriers(*pass, *state);
		state->EndRecording();

		core::Device::FrameCommandBufferData data(
			command_buffer->GetCommandBuffer(),
			vk::Semaphore(),//state->GetCurrentSemaphore(), // TODO: only provide semaphore when other pass should wait for current
			vk::Semaphore(),
			vk::PipelineStageFlagBits::eAllCommands, // TODO: fix
			core::Device::PipelineBindPoint::Graphics
		);

		data.command_buffer = command_buffer->GetCommandBuffer();
		data.queue = core::Device::PipelineBindPoint::Graphics;
		context->AddFrameCommandBuffer(data);
	}

	void TransferOwnershipQueue(ResourceWrapper& resource, PassQueue queue)
	{
		
	}

	void ApplyPreComputeBarriers(Pass& pass, VulkanRenderState& state)
	{
		for (auto* resource : pass.output_nodes)
		{
			ImageLayout target_layout = ImageLayout::Undefined;
			switch (resource->resource->type)
			{
			case ResourceType::Buffer:
			{
		

				break;
			}

			default:
				throw std::runtime_error("not implemented");
			}
		}
	}

	void RenderGraph::RecordComputePass(Pass* pass)
	{
		auto* context = Engine::GetVulkanContext();
		auto* state = context->GetRenderState();

		auto* command_buffer = state->GetCurrentCommandBuffer();
		state->BeginRecording();
		ApplyPreBarriers(*pass, *state);

		pass->record_callback(*state);

		ApplyPostBarriers(*pass, *state);
		state->EndRecording();

		core::Device::FrameCommandBufferData data(
			command_buffer->GetCommandBuffer(),
			vk::Semaphore(),
			vk::Semaphore(),
			vk::PipelineStageFlagBits::eAllCommands, // TODO: fix
			core::Device::PipelineBindPoint::Compute
		);
		context->AddFrameCommandBuffer(data);
	}

	void RenderGraph::RecordCommandBuffers()
	{
		OPTICK_EVENT();
		auto* context = Engine::GetVulkanContext();

		for (auto& pass : render_passes)
		{
			if (pass->is_compute)
				RecordComputePass(pass.get());
			else
				RecordGraphicsPass(pass.get());
		}
	}

	void RenderGraph::Render()
	{
		RecordCommandBuffers();
	}

	// TODO: remove since unused?
	vk::Semaphore RenderGraph::GetSemaphore()
	{
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
		for (auto& pair : in_flight_semaphores)
		{
			pair.first += 1;
		}

		const int in_flight_count = 2;
		auto remove_pos = std::remove_if(in_flight_semaphores.begin(), in_flight_semaphores.end(), [=](auto& pair) { return pair.first >= in_flight_count; });
		for (auto iter = remove_pos; iter != in_flight_semaphores.end(); iter++)
		{
			available_semaphores.push_back(iter->second);
		}

		in_flight_semaphores.erase(remove_pos, in_flight_semaphores.end());
	}

} } }