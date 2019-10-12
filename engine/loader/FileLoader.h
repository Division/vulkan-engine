#pragma once

#include <fstream>
#include <string>
#include <memory>
#include <vector>

namespace loader {

	// Returns empty vector in case of error
	std::vector<char> LoadFile(const std::wstring &filename);

}
