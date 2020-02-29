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
class LightObject;

namespace core
{
	namespace Device
	{
		class Texture;
	}
}

class Game : public core::IGame {
public:
	Game();
	~Game();
	void init();
	void update(float dt);
	void cleanup();

private:
	core::ECS::EntityID CreateCubeEntity(vec3 position, core::ECS::EntityID parent);

private:
	std::shared_ptr<FollowCamera> camera;
	std::shared_ptr<ModelBundle> player_model;

	core::ECS::EntityID entity1;
	core::ECS::EntityID entity2;
	core::ECS::EntityID plane;

	std::shared_ptr<Material> material_light_only;
	std::shared_ptr<Material> material_no_light;
	std::shared_ptr<Material> material_default;
	std::shared_ptr<Mesh> box_mesh;
	std::shared_ptr<Mesh> plane_mesh;
	std::shared_ptr<core::Device::Texture> lama_tex;
	std::shared_ptr<core::Device::Texture> environment;

	core::ECS::EntityManager* manager;
	core::ECS::TransformGraph* graph;

	std::shared_ptr<LightObject> light;

	bool camera_control = false;
};
