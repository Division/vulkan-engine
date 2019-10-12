#include "Game.h"
#include "CommonIncludes.h"
#include "level/Level.h"
#include "Engine.h"
#include "objects/FollowCamera.h"
#include "objects/PlayerController.h"
#include "objects/MeshObject.h"
#include "loader/ModelLoader.h"
#include "loader/HierarchyLoader.h"
#include "system/Input.h"
#include "utils/MeshGeneration.h"

Game::Game() = default;
Game::~Game() = default;

using namespace core::system;

void Game::init()
{
	level = std::make_unique<game::Level>(core::Engine::Get()->GetScene());
	level->load("resources/level/level1.mdl");
	camera = CreateGameObject<FollowCamera>();

	player_model = loader::loadModel("resources/models/dwarf/dwarf.mdl");
	auto characterIdle = loader::loadModel("resources/models/dwarf/dwarf_idle.mdl");
	auto characterRun = loader::loadModel("resources/models/dwarf/dwarf_run.mdl");
	auto characterAttackLeg = loader::loadModel("resources/models/dwarf/dwarf_attack_leg.mdl");
	player_model->appendAnimationBundle(characterRun, "run");
	player_model->appendAnimationBundle(characterIdle, "idle");
	player_model->appendAnimationBundle(characterAttackLeg, "attack_leg");

	player = loader::loadSkinnedMesh<PlayerController>(player_model);

	player->transform()->scale(vec3(0.014, 0.014, 0.014));

	camera->setPlayer(player);
	camera->setFreeCamera(true);
	
	obj1 = CreateGameObject<MeshObject>();
	obj2 = CreateGameObject<MeshObject>();

	std::shared_ptr<Mesh> mesh = std::shared_ptr<Mesh>(new Mesh());
	MeshGeneration::generateBox(mesh, 1, 1, 1);
	mesh->createBuffer();
	auto material = std::make_shared<Material>();

	obj1->mesh(mesh);
	obj1->material(material);
	obj1->transform()->position(vec3(-0.3, 0, 0));
	obj1->transform()->scale(vec3(0.6, 0.6, 0.6));
	obj2->mesh(mesh);
	obj2->material(material);
	obj2->transform()->position(vec3(3, 0, 0));
	obj2->transform()->scale(vec3(0.6, 0.6, 0.6));
	obj2->transform()->parent(obj1->transform());
}

void Game::update(float dt)
{
	obj1->transform()->rotate(vec3(0, 0, 1), M_PI * dt);
	//obj2->transform()->rotate(vec3(0, 0, 1), M_PI * dt * 2);

	auto input = core::Engine::Get()->GetInput();
	if (input->keyDown(Key::Tab)) {
		camera_control = !camera_control;
		camera->setFreeCamera(camera_control);
		player->controlsEnabled(!camera_control);
	}
}

void Game::cleanup()
{
	
}