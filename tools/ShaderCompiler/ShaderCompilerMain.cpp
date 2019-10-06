#include <cstdlib>
#include "CommonIncludes.h"

int main(int argc, char* argv[]) 
{
	auto working_dir = std::filesystem::current_path();
	std::cout << "working directory: " << working_dir << std::endl;
	return 0;
}