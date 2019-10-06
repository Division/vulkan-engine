#pragma once

#include <fstream>
#include <string>
#include <memory>
#include <vector>

namespace loader {

	// Returns null pointer in case of error
	std::shared_ptr<std::vector<char>> loadFile(const std::string &filename);

}
