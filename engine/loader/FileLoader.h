#pragma once

#include <fstream>
#include <string>
#include <memory>
#include <vector>

namespace loader {

	// Returns empty vector in case of error
	std::vector<uint8_t> LoadFile(const std::wstring &filename);

}
