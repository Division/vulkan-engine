#pragma once

#include <vector>
#include <array>
#include <glm/glm.hpp>
#include <fbxsdk.h>
#include "utils/Math.h"
#include <physx/PxPhysicsAPI.h>

using namespace glm;

namespace ozz::animation
{
	class Skeleton;
}

namespace Exporter
{
	typedef std::pair<FbxNode*, FbxMesh*> SourceMesh;

	constexpr uint32_t MESH_FILE_VERSION = 1;
	constexpr uint32_t MESH_FILE_MAGIC = 'mesh';
	constexpr uint32_t MESH_FLAG_HAS_NORMALS = 1 << 0;
	constexpr uint32_t MESH_FLAG_HAS_TBN = 1 << 1;
	constexpr uint32_t MESH_FLAG_HAS_UV0 = 1 << 2;
	constexpr uint32_t MESH_FLAG_HAS_WEIGHTS = 1 << 3;

	constexpr uint32_t PHYS_FILE_VERSION = 1;
	constexpr uint32_t PHYS_FILE_MAGIC = 'phys';

	struct MeshVertex
	{
		vec3 position = vec3(0);
		vec3 normal = vec3(0);
		vec3 binormal = vec3(0);
		vec3 tangent = vec3(0);
		vec2 uv0 = vec2(0);
		vec4 weights = vec4(0);
		ivec4 bone_indices = ivec4(0);
		int control_point_index = 0;
		bool operator==(const MeshVertex& other) const;

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
		std::string name;
		std::vector<MeshVertex> vertices;
		std::vector<uint32_t> indices;
		std::vector<uint16_t> bone_index_remap;
		std::vector<mat4> inv_bindpose;
		bool has_uv0 = false;
		bool has_normals = false;
		bool has_binormals = false;
		bool has_tangents = false;
		bool has_skinning_weights = false;
		AABB aabb;
	};

	std::vector<SubMesh> ExtractMeshes(const std::vector<SourceMesh>& meshes, FbxNode* parent_node, FbxScene* scene, ozz::animation::Skeleton* skeleton);
	bool WriteMeshToFile(const std::vector<SubMesh>& meshes, const std::wstring& filename);
	bool WritePhysMeshToFile(const std::vector<SubMesh>& meshes, const std::wstring& filename, bool is_convex, physx::PxCooking* cooking);

}