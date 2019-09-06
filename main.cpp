#include <iostream>
//#include <debugapi.h>
#include "src/Engine.h"
#include "game/Game.h"

int main(int argc, char *argv[]) {

	Game game;
	core::Engine engine(game);
	engine.MainLoop();

	std::cout << "asd";
	return 0;
}