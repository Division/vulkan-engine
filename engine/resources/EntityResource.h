#pragma once

#include "ResourceCache.h"
#include <string>
#include <vector>
#include "utils/Math.h"
#include "ecs/EntityTemplate.h"

namespace Resources
{
	class EntityResource
	{
	public:
		using Handle = Handle<EntityResource>;

		EntityResource(const std::wstring& filename);
		~EntityResource();

		size_t GetTemplateCount() const { return templates.size(); }
		const ECS::EntityTemplate* GetTemplate(size_t index) const { return templates[index].get(); }

		ECS::EntityID Spawn(vec3 position) const;

	private:
		std::vector<std::unique_ptr<ECS::EntityTemplate>> templates;
	};
}