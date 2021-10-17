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

		struct Initializer
		{
			Initializer(const std::wstring& filename) : filename(filename) {}
			Initializer(const Initializer&) = default;

			const wchar_t* GetPath() const { return filename.c_str(); }
			const uint32_t GetHash() const
			{
				uint32_t hashes[] = { FastHash(filename), (uint32_t)keep_data };
				return FastHash(hashes, sizeof(hashes));
			}

			Initializer& SetKeepData(bool value)
			{
				keep_data = value;
				return *this;
			}

			bool GetKeepData() const { return keep_data; }

		private:
			std::wstring filename;
			bool keep_data = false;
		};

		MultiMesh(const Initializer& initializer);
		MultiMesh(const std::wstring& filename, bool keep_data = false);
		~MultiMesh();

		static Handle KeepDataHandle(const std::wstring& filename) { return Handle(Initializer(filename).SetKeepData(true)); }
		static Common::Handle<MultiMesh> Create(const gsl::span<const Common::Handle<Mesh>> meshes, AABB aabb = { -vec3(1), vec3(1) });

		const size_t GetMeshCount() const { return meshes.size(); }
		const auto& GetMesh(int index) const { return meshes[index]; }
		const std::string& GetMeshName(int index) const { return mesh_names[index]; }
		const std::vector<mat4>& GetInvBindPose(int index) const { return inv_bind_poses[index]; };

	private:
		std::tuple<Common::Handle<Mesh>, std::string, std::vector<mat4>> LoadMesh(std::istream& stream);

	private:
		bool keep_data = false;
		std::vector<Common::Handle<Mesh>> meshes;
		std::vector<std::string> mesh_names;
		std::vector<std::vector<mat4>> inv_bind_poses;
		AABB aabb;
	};

}