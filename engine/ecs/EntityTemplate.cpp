#include "EntityTemplate.h"
#include "components/Transform.h"
#include "components/MultiMeshRenderer.h"
#include "ECS.h"
#include "utils/StringUtils.h"
#include "resources/MaterialResource.h"

namespace ECS
{
	
	namespace components
	{
		class TransformTemplate : public ComponentTemplate
		{
			Transform transform;
			bool skip_position = false;

			virtual void Load(const rapidjson::Value& data) override
			{
				vec3 p(0, 0, 0);

				if (data.HasMember("position"))
				{
					auto& position = data["position"];
					if (!position.IsArray())
						throw std::runtime_error("position must be an array");

					int i = 0;
					for (auto iter = position.Begin(); iter != position.End(); iter++)
					{
						if (i == 3) 
							throw std::runtime_error("position array must be of length 3");

						p[i++] = iter->GetFloat();
					}

					if (i != 3)
						throw std::runtime_error("position array must be of length 3");

					transform.position = p;
					skip_position = true;
				}
			}

			virtual void Spawn(EntityManager& manager, EntityID entity, vec3 position) override
			{
				auto* component = manager.AddComponent<Transform>(entity);
				*component = transform;

				if (!skip_position)
					component->position = position;
			}
		};

		class MultiMeshRendererTemplate : public ComponentTemplate
		{
			std::wstring path;
			std::vector<std::wstring> materials;
			Material::Handle material;

			virtual void Load(const rapidjson::Value& data) override
			{
				material = Material::Create();
				material->LightingEnabled(true);

				if (!data.HasMember("path"))
					throw std::runtime_error("path must be defined");

				path = utils::StringToWString(data["path"].GetString());

				if (data.HasMember("materials"))
				{
					auto& materials_node = data["materials"];
					if (!materials_node.IsArray())
						throw std::runtime_error("materials must be an array");

					for (auto iter = materials_node.Begin(); iter != materials_node.End(); iter++)
					{
						materials.push_back(utils::StringToWString(iter->GetString()));
					}
				}
			}

			virtual void Spawn(EntityManager& manager, EntityID entity, vec3 position) override
			{
				auto* component = manager.AddComponent<MultiMeshRenderer>(entity);
				component->multi_mesh = Resources::MultiMesh::Handle(path);

				component->material_resources = render::MaterialResourceList::Create();
				//component->materials = render::MaterialList::Create();

				for (auto& material_path : materials)
				{
					component->material_resources->emplace_back(material_path);
					//component->materials->push_back(Resources::MaterialResource::Handle(material_path)->Get());
				}
			}
		};
	}

	EntityTemplate::EntityTemplate(EntityManager& manager, const rapidjson::Value& value) : manager(manager)
	{
		if (!value.HasMember("components"))
			throw std::runtime_error("Error parsing entity: \"components\" key missing");

		auto& components = value["components"];
		if (!components.IsArray())
			throw std::runtime_error("Components must be an array");

		for (auto iter = components.Begin(); iter != components.End(); iter++)
		{
			auto& component = *iter;

			if (!component.IsObject())
				throw std::runtime_error("Components must be an object");

			if (!component.HasMember("type"))
				throw std::runtime_error("Components must have a \"type\" key");

			auto component_template = std::shared_ptr<ComponentTemplate>(manager.GetComponentTemplate(component["type"].GetString()));
			if (!component_template)
				throw std::runtime_error("Components type unknown: " + std::string(component["type"].GetString()));

			component_template->Load(component);
			component_templates.push_back(std::move(component_template));
		}
	}

	EntityID EntityTemplate::Spawn(vec3 position) const
	{
		auto entity = manager.CreateEntity();
		
		for (auto& tpl : component_templates)
		{
			tpl->Spawn(manager, entity, position);
		}

		return entity;
	}

	void RegisterEngineComponentTemplates(EntityManager& manager)
	{
		manager.RegisterComponentTemplate<components::TransformTemplate>("Transform");
		manager.RegisterComponentTemplate<components::MultiMeshRendererTemplate>("MultiMeshRenderer");
		
	}

}