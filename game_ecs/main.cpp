#include "Engine.h"
#include "Game.h"
#include "memory/Profiler.h"

int main(int argc, char* argv[]) {

	Memory::Profiler::MakeSnapshot();
	{
		Engine engine(std::make_unique<Game>());
		engine.MainLoop();
	}
	Memory::Profiler::ValidateSnapshot();

	return 0;
}