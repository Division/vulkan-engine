#pragma once

#include <scene/Behaviour.h>
#include "TopDownShooter.h"
#include "ecs/components/Transform.h"
#include "ecs/components/MultiMeshRenderer.h"

namespace scene
{

	class Ground : public Behaviour
	{
		ECS::components::Transform* transform;

	public:
		void Awake() override
		{
			transform = GetComponent<ECS::components::Transform>();
			transform->scale = vec3(10, 1, 10);

			auto renderer = GetComponent<ECS::components::MultiMeshRenderer>();
			auto material_list = render::MaterialList::Create();
			auto material = Material::Create();
			material->LightingEnabled(true);
			material->SetShaderPath(L"assets/top-down-shooter/ground/ground.hlsl");
			material->SetTexture0Resource(Resources::TextureResource::Handle(L"assets/Textures/ground/stilyzed/T_black_soil_albedo_roughness.dds"));
			material->SetNormalMapResource(Resources::TextureResource::Handle(L"assets/Textures/ground/stilyzed/T_black_soil_normal.dds"));
			material->AddExtraTexture(Resources::TextureResource::Handle(L"assets/Textures/ground/stilyzed/T_black_soil_cracks_albedo_roughness.dds"), "albedo1");
			material->AddExtraTexture(Resources::TextureResource::Handle(L"assets/Textures/ground/stilyzed/T_black_soil_cracks_normal.dds"), "normal1");
			material->AddExtraTexture(Resources::TextureResource::Handle(L"assets/Textures/ground/stilyzed/T_ground_water_albedo_roughness.dds"), "albedo2");
			material->AddExtraTexture(Resources::TextureResource::Handle(L"assets/Textures/ground/stilyzed/T_ground_water_normal.dds"), "normal2");
			material->AddExtraTexture(Resources::TextureResource::Handle(L"assets/Textures/ground/stilyzed/T_soil_water_albedo_roughness.dds"), "albedo3");
			material->AddExtraTexture(Resources::TextureResource::Handle(L"assets/Textures/ground/stilyzed/T_soil_water_normal.dds"), "normal3");
			material->AddExtraTexture(Resources::TextureResource::Handle(L"assets/top-down-shooter/ground/splatmap.dds"), "splatmap");

			material_list->push_back(material);
			renderer->materials = material_list;
		}

		void Update(float dt) override
		{
			transform->position = Game::GetInstance()->GetPlayerPosition();
			transform->position.y = 0;
		}
	};

}