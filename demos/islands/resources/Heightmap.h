#pragma once

#include "resources/ResourceCache.h"
#include <string>
#include "loader/FileLoader.h"
#include <gsl/span>

namespace Resources
{
	class Heightmap
	{
	public:
		using Handle = Handle<Heightmap>;

		Heightmap(const std::wstring& filename)
		{
			auto file = loader::LoadFile(filename);
			if (file.empty())
				throw std::runtime_error("Failed loading Heightmap");

			data.resize(file.size() / sizeof(float));
			memcpy_s(data.data(), data.size() * sizeof(float), file.data(), file.size());
		}

		gsl::span<const float> GetData() const { return data; }

	private:
		std::vector<float> data;
	};
}