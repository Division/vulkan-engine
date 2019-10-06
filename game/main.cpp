#include "Engine.h"
#include "Game.h"

int main(int argc, char* argv[]) {

	{
		core::Engine engine(std::make_unique<Game>());
		engine.MainLoop();
	}
	return 0;
}