#include "Engine.h"
#include "Game.h"
#include "memory/Profiler.h"

int main(int argc, char* argv[]) {

	core::Memory::Profiler::MakeSnapshot();
	{
		core::Engine engine(std::make_unique<Game>());
		engine.MainLoop();
	}
	core::Memory::Profiler::ValidateSnapshot();

	return 0;
}