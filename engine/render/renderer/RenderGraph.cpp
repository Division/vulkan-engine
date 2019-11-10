#include "RenderGraph.h"
#include "utils/DataStructures.h"
#include "render/device/VulkanRenderPass.h"
#include "render/device/VulkanRenderTarget.h"
#include "render/device/VulkanContext.h"
#include "render/device/VulkanSwapchain.h"
#include "render/device/VulkanRenderState.h"
#include "render/device/VkObjects.h"
#include "render/texture/Texture.h"

#include "Engine.h"

#define DEBUG_LOG true

namespace core { namespace render { namespace graph {

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
		present_node = nullptr;
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

		if (resource.type == ResourceType::Attachment && resource.GetAttachment()->IsSwapchain())
		{
			assert(!present_node);
			present_node = node;
		}

		return node;
	}

	void RenderGraph::SetCompute()
	{
		current_render_pass->is_compute = true;
	}

	void RenderGraph::Prepare()
	{
		assert(present_node);
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
			
			//if (nodes[execution_order.top()]->group == present_node->group)
				nodes[execution_order.top()]->render_pass->order = std::max(nodes[execution_order.top()]->render_pass->order, current_order);

			current_order += 1;
			execution_order.pop();
		}

		std::sort(render_passes.begin(), render_passes.end(), [](auto& a, auto& b) {
			return a->order < b->order;
		});

#if DEBUG_LOG
		for (auto& pass : render_passes)
		{
			if (pass->order == -1)
				continue;

			OutputDebugStringA("Pass ");
			OutputDebugStringA(pass->name);
			OutputDebugStringA("\n");
		}

		OutputDebugStringA("=============================\n");
#endif

	}

	std::tuple<VulkanRenderPassInitializer, VulkanRenderTargetInitializer, vec2> GetPassInitializer(Pass* pass)
	{
		VulkanRenderPassInitializer render_pass_initializer;
		VulkanRenderTargetInitializer render_target_initializer(0,0);
		vec2 size(0);

		if (pass->is_compute)
		{
			throw std::runtime_error("not implemented");
		} else
		{
			for (auto* output : pass->output_nodes)
			{
				auto* resource = output->resource;
				switch (resource->type)
				{
				case ResourceType::Attachment:
					{
						auto& attachment = *resource->GetAttachment();
						render_pass_initializer.AddAttachment(attachment);
						render_pass_initializer.SetLoadStoreOp(AttachmentLoadOp::Clear, AttachmentStoreOp::Store);

						render_target_initializer.Size(attachment.GetWidth(), attachment.GetHeight());
						size = glm::max(size, vec2(attachment.GetWidth(), attachment.GetHeight()));
						render_target_initializer.AddAttachment(attachment);

						if (resource->last_operation != ResourceWrapper::LastOperation::None)
							throw std::runtime_error("write after read/write not implemented");
						auto initial_layout = ImageLayout::Undefined;

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

					render_pass_initializer.AddAttachment(attachment);
					render_pass_initializer.SetLoadStoreOp(AttachmentLoadOp::Load, AttachmentStoreOp::DontCare);
					render_pass_initializer.SetImageLayout(resource->image_layout, ImageLayout::DepthStencilReadOnlyOptimal);

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

	enum class BarrierType
	{
		BeforeRead,
		AfterRead,
		BeforeWrite,
		AfterWrite
	};

	void TransitionImageLayout(ResourceWrapper& resource, ImageLayout layout, VulkanRenderState& state, BarrierType barrier_type)
	{
		assert(resource.type == ResourceType::Attachment);
		auto* context = Engine::GetVulkanContext();
		
		auto* attachment = resource.GetAttachment();
		bool is_swapchain = attachment->IsSwapchain();
		auto image = attachment->GetImage(context->GetCurrentFrame());
		auto& command_buffer = state.GetCurrentCommandBuffer()->GetCommandBuffer();

		bool is_color = attachment->GetType() == VulkanRenderTargetAttachment::Type::Color;

		vk::AccessFlags src_access_flags;
		vk::AccessFlags dst_access_flags;
		vk::PipelineStageFlags src_stage;
		vk::PipelineStageFlags dst_stage;

		if (is_swapchain)
		{
			switch (barrier_type) {
			case BarrierType::BeforeWrite:
				src_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
				src_access_flags = {};
				dst_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
				dst_access_flags = vk::AccessFlagBits::eColorAttachmentWrite;
				break;
			case BarrierType::AfterWrite:
				src_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
				src_access_flags = vk::AccessFlagBits::eColorAttachmentWrite;
				dst_stage = vk::PipelineStageFlagBits::eBottomOfPipe;
				dst_access_flags = vk::AccessFlagBits::eMemoryRead;
				break;
			default:
				throw std::runtime_error("invalid barrier type");
			}
		} else {
			switch (resource.last_operation)
			{
			case ResourceWrapper::LastOperation::None:
				src_access_flags = is_color ? vk::AccessFlagBits::eColorAttachmentWrite : vk::AccessFlagBits::eDepthStencilAttachmentWrite;
				src_stage = is_color ? vk::PipelineStageFlagBits::eColorAttachmentOutput : vk::PipelineStageFlagBits::eLateFragmentTests;
				break;
			case ResourceWrapper::LastOperation::Read:
				src_access_flags = is_color ? vk::AccessFlagBits::eColorAttachmentRead : vk::AccessFlagBits::eDepthStencilAttachmentRead;
				src_stage = is_color ? vk::PipelineStageFlagBits::eColorAttachmentOutput : vk::PipelineStageFlagBits::eLateFragmentTests;
				break;
			case ResourceWrapper::LastOperation::Write:
				src_access_flags = is_color ? vk::AccessFlagBits::eColorAttachmentWrite : vk::AccessFlagBits::eDepthStencilAttachmentWrite;
				src_stage = is_color ? vk::PipelineStageFlagBits::eColorAttachmentOutput : vk::PipelineStageFlagBits::eLateFragmentTests;
				break;
			}

			switch (layout)
			{
			case ImageLayout::ShaderReadOnlyOptimal:
				dst_access_flags = vk::AccessFlagBits::eShaderRead;
				dst_stage = vk::PipelineStageFlagBits::eFragmentShader; // no vertex shader reads for now
				break;

			case ImageLayout::PresentSrc:
				dst_access_flags = vk::AccessFlagBits::eColorAttachmentRead;
				dst_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
				break;

			case ImageLayout::ColorAttachmentOptimal:
				dst_access_flags = vk::AccessFlagBits::eColorAttachmentWrite;
				dst_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
				break;

			case ImageLayout::DepthStencilAttachmentOptimal:
				dst_access_flags = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
				dst_stage = vk::PipelineStageFlagBits::eLateFragmentTests;
				break;

			case ImageLayout::DepthStencilReadOnlyOptimal:
				dst_access_flags = vk::AccessFlagBits::eDepthStencilAttachmentRead;
				dst_stage = vk::PipelineStageFlagBits::eLateFragmentTests;
				break;

			default:
				throw std::runtime_error("unsupported");
			}
		}


		vk::ImageSubresourceRange range(
			is_color ? vk::ImageAspectFlagBits::eColor : vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil,
			0, 1, 0, 1
		);

		vk::ImageMemoryBarrier image_memory_barrier(
			src_access_flags, dst_access_flags, 
			(vk::ImageLayout)resource.image_layout, (vk::ImageLayout)layout, 
			VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, 
			image, range
		);

		command_buffer.pipelineBarrier(
			src_stage, 
			dst_stage,
			{}, 
			0, nullptr, 
			0, nullptr, 
			1, &image_memory_barrier
		);

#if DEBUG_LOG
		OutputDebugStringA(("[TransitionImageLayout] " + std::string("\n")).c_str());
#endif
	}

	void RenderGraph::ApplyPreBarriers(Pass& pass, VulkanRenderState& state)
	{
#if DEBUG_LOG
		OutputDebugStringA(("[ApplyPreBarriers] " + std::string(pass.name) + "\n").c_str());
#endif

		for (auto* resource : pass.output_nodes)
		{
			ImageLayout target_layout = ImageLayout::Undefined;
			switch (resource->resource->type)
			{
			case ResourceType::Attachment:
			{
				auto* attachment = resource->resource->GetAttachment();
				bool is_depth = attachment->GetType() == VulkanRenderTargetAttachment::Type::Depth;
				target_layout =  is_depth ? ImageLayout::DepthStencilAttachmentOptimal : ImageLayout::ColorAttachmentOptimal;
				
				if (resource->resource->image_layout != target_layout)
				{
					TransitionImageLayout(*resource->resource, target_layout, state, BarrierType::BeforeWrite);
					resource->resource->image_layout = target_layout;
				}
				break;
			}

			default:
				throw std::runtime_error("not implemented");
			}
		}

		for (auto node_data : pass.input_nodes)
		{
			auto* resource = node_data.first;
			ImageLayout target_layout = ImageLayout::Undefined;
			switch (resource->resource->type)
			{
			case ResourceType::Attachment:
			{
				auto* attachment = resource->resource->GetAttachment();
				auto usage = node_data.second;
				bool is_depth = attachment->GetType() == VulkanRenderTargetAttachment::Type::Depth;
				if (usage == InputUsage::DepthAttachment)
				{
					assert(is_depth);
					target_layout =  ImageLayout::DepthStencilAttachmentOptimal;
				} else
				{
					target_layout = ImageLayout::ShaderReadOnlyOptimal;
				}

				if (resource->resource->image_layout != target_layout)
				{
					TransitionImageLayout(*resource->resource, target_layout, state, BarrierType::BeforeRead);
					resource->resource->image_layout = target_layout;
				}
				break;
			}

			default:
				throw std::runtime_error("not implemented");
			}

		}
	}

	void RenderGraph::ApplyPostBarriers(Pass& pass, VulkanRenderState& state)
	{
#if DEBUG_LOG
		OutputDebugStringA(("[ApplyPostBarriers] " + std::string(pass.name) + "\n").c_str());
#endif

		BarrierType barrier_type;
		for (auto* resource : pass.output_nodes)
		{
			ImageLayout target_layout = ImageLayout::Undefined;

			switch (resource->resource->type)
			{
			case ResourceType::Attachment:
			{
				auto* attachment = resource->resource->GetAttachment();
				if (!attachment->IsSwapchain())
					return;

				barrier_type = BarrierType::AfterWrite;
				target_layout = ImageLayout::PresentSrc;
				if (resource->resource->image_layout != target_layout)
				{
					TransitionImageLayout(*resource->resource, target_layout, state, barrier_type);
					resource->resource->image_layout = target_layout;
				}
				break;
			}

			default:
				throw std::runtime_error("not implemented");
			}

		}
	}

	void RenderGraph::RecordCommandBuffers()
	{
		auto* context = Engine::GetVulkanContext();

		for (auto& pass : render_passes)
		{
			auto* state = context->GetRenderState();
			auto initializer = GetPassInitializer(pass.get());
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

			context->AddFrameCommandBuffer(command_buffer->GetCommandBuffer());
		}
	}

	void RenderGraph::Render()
	{
		RecordCommandBuffers();
	}

} } }