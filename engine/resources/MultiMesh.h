#pragma once

#include "ResourceCache.h"
#include "render/device/Resource.h"
#include <string>
#include "utils/Math.h"

class Mesh;

namespace Common
{
	template <typename T> class Handle;
}

namespace Resources
{

	class MultiMesh
	{
	public:
		using Handle = Handle<MultiMesh>;

		MultiMesh(const std::wstring& filename);
		~MultiMesh();

		const size_t GetMeshCount() const { return meshes.size(); }
		const auto& GetMesh(int index) const { return meshes[index]; }

	private:
		Common::Handle<Mesh> LoadMesh(std::istream& stream);

	private:
		std::vector<Common::Handle<Mesh>> meshes;
		AABB aabb;

	};

}