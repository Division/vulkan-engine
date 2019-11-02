#include "RenderGraph.h"
#include "utils/DataStructures.h"
#include "render/device/VulkanRenderPass.h"
#include "render/device/VulkanRenderTarget.h"

namespace core { namespace render { namespace graph {

	bool RenderGraph::ResourceRegistered(void* resource)
	{
		return std::any_of(resources.begin(), resources.end(), [=](auto& item) { return item->resource_pointer == resource; });
	}

	ResourceWrapper* RenderGraph::RegisterRenderTarget(VulkanRenderTarget& render_target)
	{
		assert(!ResourceRegistered(&render_target));
		resources.emplace_back(std::make_unique<ResourceWrapper>(ResourceType::RenderTarget, &render_target));
		return resources.back().get();
	}

	ResourceWrapper* RenderGraph::RegisterBuffer(VulkanBuffer& buffer)
	{
		assert(!ResourceRegistered(&buffer));
		resources.emplace_back(std::make_unique<ResourceWrapper>(ResourceType::RenderTarget, &buffer));
		return resources.back().get();
	}

	void RenderGraph::Clear()
	{
		render_passes.clear();
		resources.clear();
		nodes.clear();
		present_node = nullptr;
	}

	void RenderGraph::AddInput(DependencyNode& node)
	{
		current_render_pass->input_nodes.push_back(&node);
	}

	DependencyNode* RenderGraph::AddOutput(ResourceWrapper& resource)
	{
		nodes.emplace_back(std::make_unique<DependencyNode>());
		auto* node = nodes.back().get();
		node->index = nodes.size() - 1;
		node->resource = &resource;
		node->render_pass = current_render_pass;
		current_render_pass->output_nodes.push_back(node);

		if (resource.type == ResourceType::RenderTarget && resource.GetRenderTarget()->IsSwapchain())
		{
			assert(!present_node);
			present_node = node;
		}

		return node;
	}

	void RenderGraph::Prepare()
	{
		assert(present_node);
		utils::Graph graph(nodes.size());
		for (auto& pass : render_passes)
			for (auto& input : pass->input_nodes)
				for (auto& output : pass->output_nodes)
					graph.AddEdge(input->index, output->index);

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


	}

} } }