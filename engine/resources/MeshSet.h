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

	class MeshSet
	{
	public:
		using Handle = Handle<MeshSet>;

		MeshSet(const std::wstring& filename);
		~MeshSet();

		const size_t GetMeshCount() const { return meshes.size(); }
		const auto& GetMesh(int index) const { return meshes[index]; }

	private:
		Common::Handle<Mesh> LoadMesh(std::istream& stream);

	private:
		std::vector<Common::Handle<Mesh>> meshes;
		AABB aabb;

	};

}