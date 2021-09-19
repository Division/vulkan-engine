#pragma once

#include "ResourceCache.h"
#include "render/device/Resource.h"
#include <string>
#include "utils/Math.h"
#include <gsl/span>
#include <Handle.h>

class Mesh;

namespace Common
{
	template <typename T> class Handle;
}

namespace Resources
{

	class MultiMesh : public Common::Resource
	{
		MultiMesh(const gsl::span<const Common::Handle<Mesh>> meshes, AABB aabb);
	public:
		using Handle = Handle<MultiMesh>;

		MultiMesh(const std::wstring& filename);
		~MultiMesh();

		static Common::Handle<MultiMesh> Create(const gsl::span<const Common::Handle<Mesh>> meshes, AABB aabb = { -vec3(1), vec3(1) });

		const size_t GetMeshCount() const { return meshes.size(); }
		const auto& GetMesh(int index) const { return meshes[index]; }
		const std::string& GetMeshName(int index) const { return mesh_names[index]; }
		const std::vector<mat4>& GetInvBindPose(int index) const { return inv_bind_poses[index]; };

	private:
		std::tuple<Common::Handle<Mesh>, std::string, std::vector<mat4>> LoadMesh(std::istream& stream);

	private:
		std::vector<Common::Handle<Mesh>> meshes;
		std::vector<std::string> mesh_names;
		std::vector<std::vector<mat4>> inv_bind_poses;
		AABB aabb;
	};

}