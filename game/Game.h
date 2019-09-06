#pragma once

#include "Engine.h"

class Game : public core::IGame {
public:
	~Game() = default;
	void init();
	void update(float dt);
	void cleanup();

private:

};
