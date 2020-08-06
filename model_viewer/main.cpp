#include "Engine.h"
#include "ModelViewer.h"
#include "memory/Profiler.h"

int main(int argc, char* argv[]) {

	Memory::Profiler::MakeSnapshot();
	{
		Engine engine(std::make_unique<ModelViewer>());
		engine.MainLoop();
	}
	Memory::Profiler::ValidateSnapshot();

	return 0;
}