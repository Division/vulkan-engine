#include "RenderGraph.h"
#include "utils/DataStructures.h"
#include "render/device/VulkanRenderPass.h"
#include "render/device/VulkanRenderTarget.h"
#include "render/device/VulkanContext.h"
#include "render/device/VulkanSwapchain.h"
#include "render/device/VulkanRenderState.h"
#include "render/texture/Texture.h"

#include "Engine.h"

namespace core { namespace render { namespace graph {

	bool RenderGraph::ResourceRegistered(void* resource)
	{
		return std::any_of(resources.begin(), resources.end(), [=](auto& item) { return item->resource_pointer == resource; });
	}

	ResourceWrapper* RenderGraph::RegisterRenderTarget(std::shared_ptr<Texture>& render_target, bool is_depth)
	{
		assert(!ResourceRegistered(&render_target));
		auto type = is_depth ? ResourceType::DepthAttachment : ResourceType::ColorAttachment;
		resources.emplace_back(std::make_unique<ResourceWrapper>(type, &render_target));
		return resources.back().get();
	}

	ResourceWrapper* RenderGraph::RegisterBuffer(VulkanBuffer& buffer)
	{
		assert(!ResourceRegistered(&buffer));
		resources.emplace_back(std::make_unique<ResourceWrapper>(ResourceType::Buffer, &buffer));
		return resources.back().get();
	}

	ResourceWrapper* RenderGraph::RegisterSwapchain(VulkanSwapchain& swapchain)
	{
		assert(!ResourceRegistered(&swapchain));
		resources.emplace_back(std::make_unique<ResourceWrapper>(ResourceType::Swapchain, &swapchain));
		return resources.back().get();
	}

	void RenderGraph::Clear()
	{
		render_passes.clear();
		resources.clear();
		nodes.clear();
		present_node = nullptr;
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

		if (resource.type == ResourceType::Swapchain)
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

		for (auto& pass : render_passes)
		{
			if (pass->order == -1)
				continue;

			OutputDebugStringA("Pass ");
			OutputDebugStringA(pass->name);
			OutputDebugStringA("\n");
		}

		OutputDebugStringA("=============================\n");

	}

	VulkanRenderPassInitializer GetPassInitializer(Pass* pass)
	{
		VulkanRenderPassInitializer initializer;

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
				case ResourceType::ColorAttachment:
					{
						auto& texture = *resource->GetAttachment();
						initializer.AddColorAttachment(texture->GetFormat());
						initializer.SetLoadStoreOp(AttachmentLoadOp::Clear, AttachmentStoreOp::Store);

						if (resource->last_operation != ResourceWrapper::LastOperation::None)
							throw std::runtime_error("write after read/write not implemented");
						auto initial_layout = ImageLayout::Undefined;

						initializer.SetImageLayout(initial_layout, ImageLayout::ColorAttachmentOptimal);
						break;
					}

				case ResourceType::DepthAttachment:
					{
						auto& texture = *resource->GetAttachment();
						initializer.AddDepthAttachment(texture->GetFormat());
						initializer.SetLoadStoreOp(AttachmentLoadOp::Clear, AttachmentStoreOp::Store);

						if (resource->last_operation != ResourceWrapper::LastOperation::None)
							throw std::runtime_error("write after read/write not implemented");
						auto initial_layout = ImageLayout::Undefined;

						initializer.SetImageLayout(initial_layout, ImageLayout::DepthStencilAttachmentOptimal);
						break;
					}

				case ResourceType::Swapchain:
					{
						auto* swapchain = resource->GetSwapchain();

						auto* render_target = swapchain->GetRenderTarget();

						assert(resource->last_operation == ResourceWrapper::LastOperation::None);

						if (render_target->HasColor())
						{
							//initializer.AddColorAttachment(render_target->GetColorFormat());
							initializer.SetLoadStoreOp(AttachmentLoadOp::Clear, AttachmentStoreOp::Store);
						}

						if (render_target->HasDepth())
						{
							//initializer.AddDepthAttachment(render_target->GetDepthFormat());
							initializer.SetLoadStoreOp(AttachmentLoadOp::Clear, AttachmentStoreOp::Store);
						}

						initializer.SetImageLayout(ImageLayout::Undefined, ImageLayout::DepthStencilAttachmentOptimal);
						break;
					}

				default:
					throw std::runtime_error("not implemented");
				}

				for (auto& input : pass->input_nodes)
				{
					auto* resource = input.first->resource;
					auto usage = input.second;

					if (usage == InputUsage::DepthAttachment)
					{
						assert(!initializer.has_depth);
						assert(resource->type == ResourceType::DepthAttachment);
						auto& texture = *resource->GetAttachment();
						initializer.AddDepthAttachment(texture->GetFormat());
						initializer.SetLoadStoreOp(AttachmentLoadOp::Load, AttachmentStoreOp::DontCare);
					}
				}
			}
		}

		return initializer;
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

	void RenderGraph::Render()
	{
		auto* context = Engine::GetVulkanContext();

		for (auto& pass : render_passes)
		{
			auto* state = context->GetRenderState();
			auto pass_initializer = GetPassInitializer(pass.get());
			auto* vulkan_pass = GetRenderPass(pass_initializer);
			//state->BeginRendering()
		}
	}

} } }