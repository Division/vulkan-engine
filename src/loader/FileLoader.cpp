#include "FileLoader.h"

std::shared_ptr<std::vector<char>> loader::loadFile(const std::string &filename) {
	std::ifstream file(filename, std::ios::binary | std::ios::ate);
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::shared_ptr<std::vector<char>> result = std::make_shared<std::vector<char>>(size);

	if (file.read(result->data(), size)) {
		return result;
	} else {
		return nullptr;
	}
}