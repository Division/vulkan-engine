#pragma once

#include "components/Entity.h"
#include "components/Transform.h"
#include "ECS.h"
#include <unordered_map>
#include <vector>

namespace core { namespace ECS {

	class TransformGraph
	{
	public:
		struct ChildTransforms
		{
			std::vector<EntityID> children;
		};

		TransformGraph(EntityManager& manager)
			: manager(manager) 
		{
			manager.AddEntityDestroyCallback(std::bind(&TransformGraph::OnEntityDestroyed, this, std::placeholders::_1));
		}

		void AddChild(EntityID parent, EntityID child)
		{
			auto* parent_root_transform = manager.GetComponent<components::RootTransform>(parent);
			auto* parent_child_transform = manager.GetComponent<components::ChildTransform>(parent);
			auto* child_root_transform = manager.GetComponent<components::RootTransform>(child);
			auto* child_child_transform = manager.GetComponent<components::ChildTransform>(child);

			if (child_root_transform)
				manager.RemoveComponent<components::RootTransform>(child);

			// Add root transform only if have
			if (!parent_root_transform && !parent_child_transform)
				parent_root_transform = manager.AddComponent<components::RootTransform>(parent);

			if (child_child_transform)
				RemoveChild(child_child_transform->parent_id, child);
			
			child_child_transform = manager.AddComponent<components::ChildTransform>(child);

			AddChildInternal(parent, child);

			child_child_transform->parent_id = parent;
			if (parent_root_transform)
				parent_root_transform->id = parent;
		}

		void RemoveChild(EntityID parent, EntityID child)
		{
			auto* parent_root_transform = manager.GetComponent<components::RootTransform>(parent);
			auto* parent_child_transform = manager.GetComponent<components::ChildTransform>(parent);
			auto* child_root_transform = manager.GetComponent<components::RootTransform>(child);
			assert(!child_root_transform);

			auto remaining_child_count = RemoveChildInternal(parent, child);

			// Removed all children from the parent
			if (parent_root_transform && !remaining_child_count)
				manager.RemoveComponent<components::RootTransform>(parent);

			auto child_it = transforms.find(child);
			uint32_t child_children_count = 0;
			if (child_it != transforms.end())
				child_children_count = child_it->second.children.size();

			// Child is not a child anymore
			manager.RemoveComponent<components::ChildTransform>(child);

			// Children becomes root if it has own children
			if (child_children_count)
				manager.AddComponent<components::RootTransform>(child);
		}

	private:
		void OnEntityDestroyed(EntityID id)
		{
			auto it = transforms.find(id);
			if (it == transforms.end())
				return;

			auto children = it->second.children;
			for (auto child_id : children)
				RemoveChild(id, child_id);

			transforms.erase(id);
		}

		size_t RemoveChildInternal(EntityID parent, EntityID child)
		{
			auto parent_it = transforms.find(parent);
			assert(parent_it != transforms.end());
			auto& children = parent_it->second.children;

			auto child_it = std::find(children.begin(), children.end(), child);
			assert(child_it != children.end());
			auto index = child_it - children.begin();
			children[index] = children.back();
			children.pop_back();
			return children.size();
		}

		size_t AddChildInternal(EntityID parent, EntityID child)
		{
			auto parent_it = transforms.find(parent);
			if (parent_it == transforms.end())
				parent_it = transforms.insert(std::make_pair(parent, ChildTransforms())).first;

			auto& children = parent_it->second.children;
			auto child_it = std::find(children.begin(), children.end(), child);
			assert(child_it == children.end());
			children.push_back(child);
			return children.size();
		}

	private:
		EntityManager& manager;
		std::unordered_map<EntityID, ChildTransforms> transforms;
	};

} }