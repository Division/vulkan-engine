#include "RenderGraph.h"
#include "utils/DataStructures.h"

namespace core { namespace render { namespace graph {

	bool RenderGraph::ResourceRegistered(void* resource)
	{
		return std::any_of(resources.begin(), resources.end(), [=](const ResourceWrapper& item) { return item.resource_pointer == resource; });
	}

	ResourceWrapper* RenderGraph::RegisterRenderTarget(VulkanRenderTarget& render_target)
	{
		assert(!ResourceRegistered(&render_target));
		resources.push_back(ResourceWrapper(ResourceType::RenderTarget, &render_target));
		return &resources.back();
	}

	ResourceWrapper* RenderGraph::RegisterBuffer(VulkanBuffer& buffer)
	{
		assert(!ResourceRegistered(&buffer));
		resources.push_back(ResourceWrapper(ResourceType::RenderTarget, &buffer));
		return &resources.back();
	
	}

	void RenderGraph::AddPass(char* name, Pass::InitCallback init_callback, Pass::RecordCallback record_callback)
	{
		render_passes.push_back(Pass(name, init_callback, record_callback));
	}

	void RenderGraph::Clear()
	{
		render_passes.clear();
		present_node = nullptr;
	}

	void RenderGraph::AddInput(DependencyNode& node)
	{
		current_render_pass->input_nodes.push_back(&node);
	}

	DependencyNode* RenderGraph::AddOutput(ResourceWrapper& resource)
	{
		nodes.push_back(DependencyNode());
		auto* node = &nodes.back();
		node->index = nodes.size() - 1;
		node->resource = &resource;
		node->render_pass = current_render_pass;
		current_render_pass->output_nodes.push_back(node);
		return node;
	}

	void RenderGraph::SetPresentNode(DependencyNode& resource)
	{
		present_node = &resource;
	}

	void RenderGraph::Prepare()
	{
		assert(present_node);
		utils::Graph graph(nodes.size());
		for (auto& pass : render_passes)
			for (auto& input : pass.input_nodes)
				for (auto& output : pass.output_nodes)
					graph.AddEdge(input->index, output->index);

		std::stack<uint32_t> stack;
		std::stack<uint32_t> execution_order;

		std::for_each(nodes.begin(), nodes.end(), [](DependencyNode& node) {
			node.on_stack = false;
			node.visited = false;
		});

		for (int i = 0; i < nodes.size(); i++)
		{
			if (nodes[i].visited)
				continue;

			stack.push(i);

			while (!stack.empty())
			{
				uint32_t vertex = stack.top();

				if (!nodes[vertex].visited)
				{
					nodes[vertex].on_stack = true;
					nodes[vertex].visited = true;
				} else
				{
					nodes[vertex].on_stack = false;
					stack.pop();
				}

				auto& bucket = graph.GetVertexBucket(vertex);
				for (uint32_t connected_vertex : bucket)
				{
					if (!nodes[connected_vertex].visited)
						stack.push(connected_vertex);
					else if (nodes[connected_vertex].on_stack)
						throw std::runtime_error("not a DAG");
				}

				execution_order.push(vertex);
			}
		}

		int current_order = 0;
		while (!execution_order.empty())
		{
			nodes[execution_order.top()].order = current_order;
			nodes[execution_order.top()].render_pass->order = std::max(nodes[execution_order.top()].render_pass->order, current_order);
			current_order += 1;
			execution_order.pop();
		}

		// TODO: remove unused nodes
		/*std::sort(render_passes.begin(), render_passes.end(), [](const Pass& a, const Pass& b) {
			return a.order < b.order;
		});*/

	}

} } }