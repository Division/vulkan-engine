#pragma once

#include "CommonIncludes.h"
#include "utils/DataStructures.h"
#include "ecs/components/Entity.h"
#include "rapidjson/document.h"

namespace ECS
{
	class EntityManager;

	class ComponentTemplate
	{
	public:
		virtual void Load(const rapidjson::Value& data) = 0;
		virtual void Spawn(EntityManager& manager, EntityID entity, vec3 position) = 0;

		~ComponentTemplate() = default;
	};

	class EntityTemplate
	{
	public:
		EntityID Spawn(vec3 position) const;
		EntityTemplate(EntityManager& manager, const rapidjson::Value& value);

	private:
		std::vector<std::shared_ptr<ComponentTemplate>> component_templates;
		EntityManager& manager;
	};

	void RegisterEngineComponentTemplates(EntityManager& manager);
}