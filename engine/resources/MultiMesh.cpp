#include <fstream>
#include <limits>

#include "MultiMesh.h"
#include "render/mesh/Mesh.h"
#include "utils/StringUtils.h"

namespace Resources
{
	MultiMesh::MultiMesh(const Initializer& initializer) : MultiMesh(initializer.GetPath(), initializer.GetKeepData())
	{
	}

	MultiMesh::MultiMesh(const std::wstring& filename, bool keep_data)
		: keep_data(keep_data)
	{
		std::ifstream stream(filename, std::ios::binary);
		stream.exceptions(std::ios::badbit | std::ios::failbit);

		uint32_t magic, version, mesh_count;
		stream.read((char*)&magic, sizeof(magic));


		if (magic != 'mesh')
			throw std::runtime_error("Not a mesh");

		stream.read((char*)&version, sizeof(version));
		stream.read((char*)&mesh_count, sizeof(mesh_count));
		stream.read((char*)&aabb, sizeof(aabb));

		const auto debug_name = utils::WStringToString(filename);

		for (int i = 0; i < mesh_count; i++)
		{
			auto [mesh, name, inv_bind_pose] = LoadMesh(stream, debug_name);
			if (!mesh)
				throw Exception(filename) << "failed loading mesh";
			
			meshes.push_back(mesh);
			mesh_names.push_back(std::move(name));
			inv_bind_poses.push_back(inv_bind_pose);
		}
	}

	MultiMesh::MultiMesh(const gsl::span<const Common::Handle<Mesh>> in_meshes, AABB aabb)
	{
		for (auto& mesh : in_meshes)
		{
			meshes.push_back(mesh);
			mesh_names.push_back("mesh");
		}

		this->aabb = aabb;
	}

	Common::Handle<MultiMesh> MultiMesh::Create(const gsl::span<const Common::Handle<Mesh>> meshes, AABB aabb)
	{
		return Common::Handle<MultiMesh>(std::unique_ptr<MultiMesh>(new MultiMesh(meshes, aabb)));
	}

	std::tuple<Common::Handle<Mesh>, std::string, std::vector<mat4>> MultiMesh::LoadMesh(std::istream& stream, const std::string& debug_name)
	{
		uint32_t flags, vertex_count, triangle_count;
		AABB mesh_aabb;
		std::vector<mat4> inv_bind_pose;

		uint8_t name_length;
		std::string name;

		stream.read((char*)&flags, sizeof(flags));
		stream.read((char*)&vertex_count, sizeof(vertex_count));
		stream.read((char*)&triangle_count, sizeof(triangle_count));
		stream.read((char*)&mesh_aabb, sizeof(mesh_aabb));
		stream.read((char*)&name_length, sizeof(name_length));
		name.resize(name_length);
		stream.read(name.data(), name_length);

		std::vector<uint8_t> index_data;
		std::vector<uint8_t> vertex_data;
		std::vector<uint16_t> bone_remap_data;

		if (flags & Mesh::MESH_FLAG_HAS_WEIGHTS)
		{
			uint16_t bone_remap_size;
			stream.read((char*)&bone_remap_size, sizeof(bone_remap_size));

			assert(bone_remap_size < 1000);
			bone_remap_data.resize(bone_remap_size);
			stream.read((char*)bone_remap_data.data(), bone_remap_size * sizeof(uint16_t));
			inv_bind_pose.resize(bone_remap_size);
			stream.read((char*)inv_bind_pose.data(), bone_remap_size * sizeof(mat4));
		}

		const uint32_t index_count = triangle_count * 3;
		const bool use_short_indices = Mesh::IsShortIndexCount(index_count);
		const size_t index_size = use_short_indices ? sizeof(uint16_t) : sizeof(uint32_t);
		index_data.resize(index_count * index_size);
		stream.read((char*)index_data.data(), index_data.size());

		size_t vertex_stride = Mesh::GetVertexStride(flags);
		vertex_data.resize(vertex_count * vertex_stride);
		stream.read((char*)vertex_data.data(), vertex_data.size());

		auto mesh = Mesh::Create(flags, vertex_data.data(), vertex_count, index_data.data(), triangle_count, mesh_aabb, keep_data, debug_name);
		if (bone_remap_data.size())
			mesh->SetBoneRemap(bone_remap_data.data(), bone_remap_data.size());

		return std::make_tuple(mesh, name, inv_bind_pose);
	}

	MultiMesh::~MultiMesh()
	{

	}
}
