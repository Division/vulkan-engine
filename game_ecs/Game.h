#pragma once

#include "Engine.h"
#include "ecs/ECS.h"
#include "ecs/TransformGraph.h"
#include "ecs/systems/TransformSystem.h"
#include "ecs/systems/RendererSystem.h"

namespace game
{
	class Level;
}

class Mesh;
class FollowCamera;
class ModelBundle;
class MeshObject;
class PlayerController;
class Material;

class Game : public core::IGame {
public:
	Game();
	~Game();
	void init();
	void update(float dt);
	void cleanup();

private:
	core::ECS::EntityID CreateCubeEntity(vec3 position, core::ECS::EntityID parent);
	void ProcessTransformSystems();
	void ProcessRendererSystems();

private:
	std::shared_ptr<FollowCamera> camera;
	std::shared_ptr<ModelBundle> player_model;

	core::ECS::EntityID entity1;
	core::ECS::EntityID entity2;

	std::shared_ptr<Material> material_no_light;
	std::shared_ptr<Material> material_default;
	std::shared_ptr<Mesh> box_mesh;

	std::unique_ptr<core::ECS::EntityManager> manager;
	std::unique_ptr<core::ECS::TransformGraph> graph;
	std::unique_ptr<core::ECS::systems::NoChildTransformSystem> no_child_system;
	std::unique_ptr<core::ECS::systems::RootTransformSystem> root_transform_system;
	std::unique_ptr<core::ECS::systems::UpdateRendererSystem> update_renderer_system;

	bool camera_control = false;
};
