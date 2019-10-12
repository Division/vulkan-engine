#pragma once

#include "Engine.h"

namespace game
{
	class Level;
}

class FollowCamera;
class ModelBundle;
class MeshObject;
class PlayerController;

class Game : public core::IGame {
public:
	Game();
	~Game();
	void init();
	void update(float dt);
	void cleanup();

private:
	std::unique_ptr<game::Level> level;
	std::shared_ptr<FollowCamera> camera;
	std::shared_ptr<PlayerController> player;
	std::shared_ptr<ModelBundle> player_model;
	std::shared_ptr<MeshObject> obj1;
	std::shared_ptr<MeshObject> obj2;
	bool camera_control = false;
};
