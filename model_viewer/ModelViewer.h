﻿#pragma once

#include "Engine.h"
#include "ecs/ECS.h"
#include "ecs/TransformGraph.h"
#include "ecs/systems/TransformSystem.h"
#include "ecs/systems/RendererSystem.h"
#include "resources/ResourceCache.h"

class Mesh;
class FollowCamera;
class ModelBundle;
class MeshObject;
class PlayerController;
class Material;
class LightObject;
class ViewerCamera;

namespace Device
{
	class Texture;
}

class ModelViewer : public IGame {
public:
	ModelViewer();
	~ModelViewer();
	void init();
	void update(float dt);
	void cleanup();

private:
	ECS::EntityID CreateMeshEntity(vec3 position, ECS::EntityID parent, Mesh* mesh);

private:
	std::unique_ptr<ViewerCamera> camera;
	std::shared_ptr<ModelBundle> player_model;

	ECS::EntityID entity1;
	ECS::EntityID entity2;
	ECS::EntityID plane;

	std::shared_ptr<Material> material_light_only;
	std::shared_ptr<Material> material_no_light;
	std::shared_ptr<Material> material_default;
	std::shared_ptr<Mesh> plane_mesh;
	std::shared_ptr<Mesh> sphere_mesh;
	Resources::Handle<ModelBundle> sphere_bundle;
	std::shared_ptr<Device::Texture> lama_tex;
	std::shared_ptr<Device::Texture> environment;

	ECS::EntityManager* manager;
	ECS::TransformGraph* graph;

	bool camera_control = false;
};