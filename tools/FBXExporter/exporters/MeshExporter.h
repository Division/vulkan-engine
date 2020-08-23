#pragma once

#include <vector>
#include <array>
#include <glm/glm.hpp>
#include <fbxsdk.h>
#include "utils/Math.h"

using namespace glm;

namespace Exporter
{

	constexpr uint32_t MESH_FILE_VERSION = 1;
	constexpr uint32_t MESH_FILE_MAGIC = 'mesh';
	constexpr uint32_t MESH_FLAG_HAS_NORMALS = 1 << 0;
	constexpr uint32_t MESH_FLAG_HAS_TBN = 1 << 1;
	constexpr uint32_t MESH_FLAG_HAS_UV0 = 1 << 2;
	constexpr uint32_t MESH_FLAG_HAS_WEIGHTS = 1 << 3;

	struct MeshVertex
	{
		vec3 position;
		vec3 normal;
		vec3 binormal;
		vec3 tangent;
		vec2 uv0;
		vec4 weights;
		vec4 bone_indices;

		bool operator==(const MeshVertex& other) const = default;

		uint32_t GetHash() const;
	};

	struct MeshTriangle
	{
		std::array<MeshVertex, 3> vertices;
	};

	struct Material
	{
		std::string name;
		std::string diffuse_texture;
		std::string normal_texture;
		vec4 diffuse_color;
	};

	struct SubMesh
	{
		Material material;
		std::vector<MeshVertex> vertices;
		std::vector<uint32_t> indices;
		bool has_uv0 = false;
		bool has_normals = false;
		bool has_binormals = false;
		bool has_tangents = false;
		bool has_skinning_weights = false;
		AABB aabb;
	};

	std::vector<SubMesh> ExtractMeshes(const std::vector<FbxMesh*>& meshes);
	bool WriteMeshToFile(const std::vector<SubMesh>& meshes, const std::wstring& filename);

}