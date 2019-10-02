#pragma once

#include "Engine.h"

namespace game
{
	class Level;
}

class Game : public core::IGame {
public:
	Game();
	~Game();
	void init();
	void update(float dt);
	void cleanup();

private:
	std::unique_ptr<game::Level> level;
};
