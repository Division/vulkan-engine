#include <fstream>
#include <limits>

#include "MeshSet.h"
#include "render/mesh/Mesh.h"

namespace Resources
{
	MeshSet::MeshSet(const std::wstring& filename)
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

		for (int i = 0; i < mesh_count; i++)
		{
			auto mesh = LoadMesh(stream);
			if (!mesh)
				throw Exception(filename) << "failed loading mesh";
			
			meshes.push_back(mesh);
		}
	}

	Common::Handle<Mesh> MeshSet::LoadMesh(std::istream& stream)
	{
		uint32_t flags, vertex_count, triangle_count;
		AABB mesh_aabb;

		stream.read((char*)&flags, sizeof(flags));
		stream.read((char*)&vertex_count, sizeof(vertex_count));
		stream.read((char*)&triangle_count, sizeof(triangle_count));
		stream.read((char*)&mesh_aabb, sizeof(mesh_aabb));

		std::vector<uint8_t> index_data;
		std::vector<uint8_t> vertex_data;

		const uint32_t index_count = triangle_count * 3;
		const bool use_short_indices = Mesh::IsShortIndexCount(index_count);
		const size_t index_size = use_short_indices ? sizeof(uint16_t) : sizeof(uint32_t);
		index_data.resize(index_count * index_size);
		stream.read((char*)index_data.data(), index_data.size());
		
		size_t vertex_stride = Mesh::GetVertexStride(flags);
		vertex_data.resize(vertex_count * vertex_stride);
		stream.read((char*)vertex_data.data(), vertex_data.size());

		return Mesh::Create(flags, vertex_data.data(), vertex_count, index_data.data(), triangle_count, mesh_aabb);
	}

	MeshSet::~MeshSet()
	{

	}
}
