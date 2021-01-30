#pragma once

#include "EntityResource.h"
#include "loader/FileLoader.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "utils/StringUtils.h"
#include "system/Logging.h"
#include "Engine.h"
#include "ecs/ECS.h"
#include "ecs/components/Entity.h"

namespace Resources
{

	EntityResource::EntityResource(const std::wstring& filename)
	{
		auto data = loader::LoadFile(filename);

		if (!data.size())
		{
			throw Exception(filename) << "Error reading entity data";
		}

		rapidjson::Document json;
		json.Parse((char*)data.data(), data.size());
		if (json.HasParseError() || !json.IsArray())
		{
			throw Exception(filename) << "Error parsing entity";
		}

		auto* manager = Engine::Get()->GetEntityManager();

		auto arr = json.GetArray();

		for (auto iter = arr.Begin(); iter != arr.End(); iter++)
		{
			if (!iter->IsObject())
				throw Exception(filename) << "Error parsing entity";

			auto& entity_data = *iter;

			if (!entity_data.IsObject())
				throw Exception(filename) << "Error parsing entity";

			templates.push_back(std::make_unique<ECS::EntityTemplate>(*manager, entity_data));
		}
	}

	ECS::EntityID EntityResource::Spawn(vec3 position) const
	{
		auto& manager = *Engine::Get()->GetEntityManager();

		ECS::components::RootEntity* root_entity = nullptr;
		ECS::EntityID root_entity_id = 0;

		for (size_t i = 0; i < GetTemplateCount(); i++)
		{
			auto* tpl = GetTemplate(i);
			auto entity = tpl->Spawn(position);
			if (i == 0)
			{
				root_entity = manager.AddComponent<ECS::components::RootEntity>(entity);
				root_entity_id = entity;
			}
			else
			{
				//root_entity->entities.push_back(entity);
			}
		}

		return root_entity_id;
	}

	EntityResource::~EntityResource()
	{

	}

}