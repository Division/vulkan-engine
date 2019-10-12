#include "FileLoader.h"

namespace loader 
{
	std::vector<char> LoadFile(const std::wstring& filename) 
	{
		std::ifstream file(filename, std::ios::binary | std::ios::ate);
		file.exceptions(std::ios::failbit);
		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		auto result = std::vector<char>(size);

		if (file.read(result.data(), size)) {
			return result;
		}
		else {
			return std::vector<char>();
		}
	}

}