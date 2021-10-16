#include "EntityTemplate.h"
#include "components/Transform.h"
#include "components/MultiMeshRenderer.h"
#include "components/AnimationController.h"
#include "components/Physics.h"
#include "ECS.h"
#include "utils/StringUtils.h"
#include "utils/JsonUtils.h"
#include "resources/MaterialResource.h"
#include "resources/SkeletonResource.h"
#include "scene/PhysicsHelper.h"
#include "scene/Scene.h"
#include "Engine.h"

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
				if (data.HasMember("position"))
				{
					transform.position = utils::JSON::ReadVector3(data["position"]);
					skip_position = true;
				}

				if (data.HasMember("scale"))
				{
					transform.scale = utils::JSON::ReadVector3(data["scale"]);
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
			std::map<std::string, std::wstring> material_map;
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
					if (materials_node.IsArray())
					{
						for (auto iter = materials_node.Begin(); iter != materials_node.End(); iter++)
						{
							materials.push_back(utils::StringToWString(iter->GetString()));
						}
					}
					else if (materials_node.IsObject())
					{
						auto obj = materials_node.GetObjectA();
						std::wstring default_material;
						for (auto iter = obj.MemberBegin(); iter != obj.MemberEnd(); iter++)
						{
							std::string mesh_name = iter->name.GetString();
							utils::Lowercase(mesh_name);
							material_map[mesh_name] = utils::StringToWString(iter->value.GetString());
						}
					}
					else
						throw std::runtime_error("materials must be array or object");
				}
			}

			virtual void Spawn(EntityManager& manager, EntityID entity, vec3 position) override
			{
				auto* component = manager.AddComponent<MultiMeshRenderer>(entity);
				component->SetMultiMesh(Resources::MultiMesh::Handle(path));

				component->material_resources = render::MaterialResourceList::Create();

				if (materials.empty() && !material_map.empty())
				{
					const auto default_iter = material_map.find("default");
					auto default_material = default_iter != material_map.end() ? default_iter->second : material_map.begin()->second;

					for (int i = 0; i < component->GetMultiMesh()->GetMeshCount(); i++)
					{
						std::string mesh_name = component->GetMultiMesh()->GetMeshName(i);
						utils::Lowercase(mesh_name);
						auto iter = material_map.find(mesh_name);
						if (iter != material_map.end())
							materials.push_back(iter->second);
						else
							materials.push_back(default_material);
					}
				}

				for (auto& material_path : materials)
				{
					component->material_resources->emplace_back(material_path);
				}

				AABB combined_aabb;
				if (component->GetMultiMesh()->GetMeshCount())
					combined_aabb = component->GetMultiMesh()->GetMesh(0)->aabb();

				for (int i = 1; i < component->GetMultiMesh()->GetMeshCount(); i++)
				{
					const auto mesh_aabb = component->GetMultiMesh()->GetMesh(i)->aabb();
					combined_aabb.expand(mesh_aabb.min);
					combined_aabb.expand(mesh_aabb.max);
				}

				auto* transform = manager.GetComponent<components::Transform>(entity);
				if (transform)
					transform->bounds = combined_aabb;
			}
		};

		class AnimationControllerTemplate : public ComponentTemplate
		{
			std::wstring skeleton;

			virtual void Load(const rapidjson::Value& data) override
			{
				if (!data.HasMember("skeleton"))
					throw std::runtime_error("skeleton must be defined");

				skeleton = utils::StringToWString(data["skeleton"].GetString());
			}

			virtual void Spawn(EntityManager& manager, EntityID entity, vec3 position) override
			{
				manager.AddComponent<AnimationController>(entity, Resources::SkeletonResource::Handle(skeleton));
			}
		};

		class PhysicsCharacterControllerTemplate : public ComponentTemplate
		{
			float radius = 1.0f;
			float height = 1.0f;

			virtual void Load(const rapidjson::Value& data) override
			{
				if (data.HasMember("radius"))
				{
					if (!data["radius"].IsNumber())
						throw std::runtime_error("radius must be a number");
					radius = data["radius"].GetFloat();
				}

				if (data.HasMember("height"))
				{
					if (!data["height"].IsNumber())
						throw std::runtime_error("height must be a number");
					height = data["height"].GetFloat();
				}

			}

			virtual void Spawn(EntityManager& manager, EntityID entity, vec3 position) override
			{
				auto phys_controller = manager.AddComponent<PhysXCharacterController>(entity);
				phys_controller->controller = Engine::Get()->GetScene()->GetPhysics()->CreateCapsuleController(position, vec3(0, 1, 0), height, radius);
			}
		};

		class PhysBodyTemplate : public ComponentTemplate
		{
			Physics::Helper::PhysicsInitializer initializer;

			virtual void Load(const rapidjson::Value& data) override
			{
				if (!data.HasMember("shape"))
					throw std::runtime_error("shape must be defined");

				auto& shape = data["shape"];
				if (!shape.IsString())
					throw std::runtime_error("shape must be a string");

				const bool has_size = data.HasMember("size");
				auto& size = data["size"];

				const bool dynamic = data.HasMember("dynamic") && data["dynamic"].GetBool();

				initializer.is_static = !dynamic;

				if (shape.GetString() == std::string("sphere"))
				{
					initializer.shape = Physics::Helper::PhysicsInitializer::Shape::Sphere;
					if (!has_size || !size.IsNumber())
						throw std::runtime_error("size is missing or must be a number");

					initializer.size.x = size.GetFloat();
				}
				else if (shape.GetString() == std::string("box"))
				{
					initializer.shape = Physics::Helper::PhysicsInitializer::Shape::Box;
					if (!has_size || !(size.IsNumber() || size.IsArray()))
						throw std::runtime_error("size is missing or must be a number or array");

					if (size.IsNumber())
						initializer.size = vec3(size.GetFloat() / 2.0f);
					else
						initializer.size = utils::JSON::ReadVector3(size);
				}
				else if (shape.GetString() == std::string("plane"))
				{
					initializer.shape = Physics::Helper::PhysicsInitializer::Shape::Plane;
				}
				else
				{
					throw std::runtime_error("unknown shape");
				}
			}

			virtual void Spawn(EntityManager& manager, EntityID entity, vec3 position) override
			{
				auto actial_initializer = initializer;
				actial_initializer.position = position;

				auto transform = manager.GetComponent<Transform>(entity);
				if (transform)
					actial_initializer.size *= transform->scale;
				Physics::Helper::AddPhysics(manager, entity, actial_initializer);
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
		manager.RegisterComponentTemplate<components::PhysBodyTemplate>("PhysBody");
		manager.RegisterComponentTemplate<components::AnimationControllerTemplate>("AnimationController");
		manager.RegisterComponentTemplate<components::PhysicsCharacterControllerTemplate>("PhysicsCharacterController");
	}

}