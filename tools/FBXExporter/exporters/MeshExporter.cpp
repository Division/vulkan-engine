#include <unordered_map>
#include <filesystem>
#include <fstream>
#include "MeshExporter.h"
#include "scene/Physics.h"
//#include "utils/Math.h"

namespace fs = std::filesystem;
using namespace physx;
using namespace fbxsdk;

namespace Exporter
{
	uint32_t MeshVertex::GetHash() const
	{
		return FastHash(this, sizeof(*this));
	}

	Material ExtractMaterial(FbxMesh* mesh)
	{
		Material result;

		if (mesh->GetElementMaterial())
		{
			auto material_indices = &mesh->GetElementMaterial()->GetIndexArray();
			auto mapping_mode = mesh->GetElementMaterial()->GetMappingMode();
		}

		return result;
	}

	void AddVertices(SubMesh& mesh, const std::vector<MeshTriangle>& triangles)
	{
		struct Hasher
		{
			size_t operator()(const MeshVertex& vertex) const { return vertex.GetHash(); }
		};

		std::unordered_map<MeshVertex, uint32_t, Hasher> index_map;

		mesh.aabb = AABB(triangles[0].vertices[0].position, triangles[0].vertices[0].position);

		uint32_t index = 0;
		for (auto& triangle : triangles)
		{
			for (int i = 0; i < triangle.vertices.size(); i++)
			{
				auto& vertex = triangle.vertices[i];
				mesh.aabb.expand(vertex.position);

				auto it = index_map.find(vertex);
				if (it == index_map.end())
				{
					index_map.insert(std::make_pair(vertex, index));
					mesh.vertices.push_back(vertex);
					mesh.indices.push_back(index);
					index += 1;
				}
				else
				{
					mesh.indices.push_back(it->second);
				}
			}
		}
	}

	std::vector<SubMesh> ExtractMeshes(const std::vector<SourceMesh>& meshes, FbxNode* parent_node)
	{
		std::vector<SubMesh> result;

		// Maps material id to triangle list
		std::unordered_map<FbxGeometryElementMaterial*, std::vector<MeshTriangle>> material_triangles;

		auto root_node_inv_transform = parent_node ? parent_node->EvaluateGlobalTransform() : fbxsdk::FbxAMatrix();
		root_node_inv_transform = fbxsdk::FbxAMatrix(-root_node_inv_transform.GetT(), FbxVector4(), FbxVector4(1,1,1,1)); // Inverse translation only

		for (auto& mesh_pair : meshes)
		{
			auto* mesh = mesh_pair.second;

			// Transformation related to root node
			auto mesh_transform = root_node_inv_transform * mesh_pair.first->EvaluateGlobalTransform();

			if (!mesh->IsTriangleMesh())
				throw std::runtime_error("Mesh has non-triangle polygons: " + std::string(mesh->GetName()));

			auto* material = mesh->GetElementMaterial();
			auto& triangles = material_triangles[material];
			auto& sub_mesh = result.emplace_back();

			sub_mesh.has_normals = mesh->GetElementNormalCount() > 0;
			sub_mesh.has_binormals = mesh->GetElementBinormalCount() > 0;
			sub_mesh.has_tangents = mesh->GetElementTangentCount() > 0;
			sub_mesh.has_skinning_weights = false;
			sub_mesh.has_uv0 = mesh->GetElementUVCount() > 0;
			FbxGeometryElementBinormal* binormal_element = sub_mesh.has_binormals ? mesh->GetElementBinormal(0) : nullptr;
			auto binormal_reference_mode = sub_mesh.has_binormals ? binormal_element->GetReferenceMode() : FbxGeometryElement::eDirect;
			if (binormal_element && binormal_element->GetMappingMode() != FbxGeometryElement::eByPolygonVertex)
				throw std::runtime_error("Binormal mapping mode must be FbxGeometryElement::eByPolygonVertex: " + std::string(mesh->GetName()));
			
			FbxGeometryElementTangent* tangent_element = sub_mesh.has_tangents ? mesh->GetElementTangent(0) : nullptr;
			auto tangent_reference_mode = sub_mesh.has_tangents ? tangent_element->GetReferenceMode() : FbxGeometryElement::eDirect;
			if (tangent_element && tangent_element->GetMappingMode() != FbxGeometryElement::eByPolygonVertex)
				throw std::runtime_error("Tangent mapping mode must be FbxGeometryElement::eByPolygonVertex: " + std::string(mesh->GetName()));

			FbxStringList UVNames;
			mesh->GetUVSetNames(UVNames);
			const char* uv_name = NULL;
			if (sub_mesh.has_uv0 && UVNames.GetCount())
			{
				uv_name = UVNames[0];
			}

			const int triangle_count = mesh->GetPolygonCount();
			for (int i = 0; i < triangle_count; i++)
			{
				auto& triangle = triangles.emplace_back();
				for (int v = 0; v < triangle.vertices.size(); v++)
				{
					const int index = mesh->GetPolygonVertex(i, v);
					FbxVector4 fbx_vertex = mesh->GetControlPoints()[index];
					fbx_vertex = mesh_transform.MultT(fbx_vertex);
					triangle.vertices[v].position = vec3(fbx_vertex[0], fbx_vertex[1], fbx_vertex[2]);
				}

				if (sub_mesh.has_normals)
				{
					for (int v = 0; v < triangle.vertices.size(); v++)
					{
						FbxVector4 fbx_normal;
						mesh->GetPolygonVertexNormal(i, v, fbx_normal);
						fbx_normal[3] = 0;
						fbx_normal = mesh_transform.MultT(fbx_normal);
						triangle.vertices[v].normal = vec3(fbx_normal[0], fbx_normal[1], fbx_normal[2]);
					}
				}

				if (sub_mesh.has_uv0)
				{
					for (int v = 0; v < triangle.vertices.size(); v++)
					{
						FbxVector2 fbx_uv;
						bool unmapped;
						mesh->GetPolygonVertexUV(i, v, uv_name, fbx_uv, unmapped);
						triangle.vertices[v].uv0 = vec2(fbx_uv[0], fbx_uv[1]);
					}
				}

				if (sub_mesh.has_binormals)
				{
					for (int v = 0; v < triangle.vertices.size(); v++)
					{
						const int index = mesh->GetPolygonVertex(i, v);
						FbxVector4 fbx_binormal;


						switch (binormal_reference_mode)
						{
							case FbxGeometryElement::eDirect:
								fbx_binormal = binormal_element->GetDirectArray().GetAt(i * 3 + v);
								break;
							case FbxGeometryElement::eIndexToDirect:
							{
								int id = binormal_element->GetIndexArray().GetAt(i * 3 + v);
								fbx_binormal = binormal_element->GetDirectArray().GetAt(id);
								break;
							}
							default:
								throw std::runtime_error("Mesh binormal reference mode unsupported: " + std::string(mesh->GetName()));
						}

						fbx_binormal[3] = 0;
						fbx_binormal = mesh_transform.MultT(fbx_binormal);
						triangle.vertices[v].binormal = vec3(fbx_binormal[0], fbx_binormal[1], fbx_binormal[2]);
					}
				}

				if (sub_mesh.has_tangents)
				{
					for (int v = 0; v < triangle.vertices.size(); v++)
					{
						const int index = mesh->GetPolygonVertex(i, v);
						FbxVector4 fbx_tangent;
						switch (tangent_reference_mode)
						{
						case FbxGeometryElement::eDirect:
							fbx_tangent = tangent_element->GetDirectArray().GetAt(i * 3 + v);
							break;
						case FbxGeometryElement::eIndexToDirect:
						{
							int id = tangent_element->GetIndexArray().GetAt(i * 3 + v);
							fbx_tangent = tangent_element->GetDirectArray().GetAt(id);
							break;
						}
						default:
							throw std::runtime_error("Mesh tangent reference mode unsupported: " + std::string(mesh->GetName()));
						}

						fbx_tangent[3] = 0;
						fbx_tangent = mesh_transform.MultT(fbx_tangent);
						triangle.vertices[v].tangent = vec3(fbx_tangent[0], fbx_tangent[1], fbx_tangent[2]);
					}
				}
			}

			AddVertices(sub_mesh, triangles);
		}

		return result;
	}

	void WriteVertex(std::ostream& stream, const SubMesh& submesh, uint32_t vertex_index)
	{
		if (vertex_index >= submesh.vertices.size())
			throw std::runtime_error("Vertex index out of range");

		auto& vertex = submesh.vertices[vertex_index];
		stream.write((char*)&vertex.position, sizeof(vertex.position));
		
		if (submesh.has_normals)
		{
			stream.write((char*)&vertex.normal, sizeof(vertex.normal));
		}
		
		if (submesh.has_tangents && submesh.has_binormals)
		{
			stream.write((char*)&vertex.binormal, sizeof(vertex.binormal));
			stream.write((char*)&vertex.tangent, sizeof(vertex.tangent));
		}

		if (submesh.has_uv0)
		{
			stream.write((char*)&vertex.uv0, sizeof(vertex.uv0));
		}

		if (submesh.has_skinning_weights)
		{
			stream.write((char*)&vertex.bone_indices, sizeof(vertex.bone_indices));
			stream.write((char*)&vertex.weights, sizeof(vertex.weights));
		}
	}

	void WriteSubMesh(std::ostream& stream, const SubMesh& submesh)
	{
		uint32_t flags = 0;
		if (submesh.has_normals) flags |= MESH_FLAG_HAS_NORMALS;
		if (submesh.has_binormals && submesh.has_tangents) flags |= MESH_FLAG_HAS_TBN;
		if (submesh.has_uv0) flags |= MESH_FLAG_HAS_UV0;
		if (submesh.has_skinning_weights) flags |= MESH_FLAG_HAS_WEIGHTS;

		const uint32_t vertex_count = submesh.vertices.size();
		const uint32_t index_count = submesh.indices.size();
		if (index_count % 3 != 0)
			throw std::runtime_error("Mesh index count must be divisable by 3");

		const uint32_t triangle_count = index_count / 3;
		const bool use_short_indices = index_count <= std::numeric_limits<uint16_t>::max();

		stream.write((char*)&flags, sizeof(flags));
		stream.write((char*)&vertex_count, sizeof(vertex_count));
		stream.write((char*)&triangle_count, sizeof(triangle_count));
		stream.write((char*)&submesh.aabb, sizeof(submesh.aabb));

		if (use_short_indices)
		{
			std::vector<uint16_t> short_indices(index_count);
			for (int i = 0; i < index_count; i++)
				short_indices[i] = submesh.indices[i];
			stream.write((char*)short_indices.data(), index_count * sizeof(uint16_t));
		}
		else
			stream.write((char*)submesh.indices.data(), index_count * sizeof(uint32_t));

		for (int i = 0; i < vertex_count; i++)
			WriteVertex(stream, submesh, i);
	}

	bool WriteMeshToFile(const std::vector<SubMesh>& meshes, const std::wstring& filename)
	{
		fs::path path = filename;
		fs::create_directories(path.parent_path());

		const uint32_t sub_mesh_count = meshes.size();

		AABB total_aabb = meshes[0].aabb;
		for (int i = 1; i < meshes.size(); i++)
		{
			total_aabb.expand(meshes[i].aabb.min);
			total_aabb.expand(meshes[i].aabb.max);
		}

		std::ofstream stream(filename, std::ios::binary);
		stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		stream.write((char*)&MESH_FILE_MAGIC, sizeof(MESH_FILE_MAGIC));
		stream.write((char*)&MESH_FILE_VERSION, sizeof(MESH_FILE_VERSION));
		stream.write((char*)&sub_mesh_count, sizeof(sub_mesh_count));
		stream.write((char*)&total_aabb, sizeof(total_aabb));

		for (auto& mesh : meshes)
			WriteSubMesh(stream, mesh);

		return stream.good();
	}

	void WritePhysMesh(std::ostream& stream, const SubMesh& submesh, bool is_convex, PxCooking* cooking)
	{
		const uint32_t vertex_count = submesh.vertices.size();
		const uint32_t index_count = submesh.indices.size();
		if (index_count % 3 != 0)
			throw std::runtime_error("Mesh index count must be divisable by 3");

		const uint32_t triangle_count = index_count / 3;
		const bool use_short_indices = index_count <= std::numeric_limits<uint16_t>::max();
		uint8_t convex = is_convex;

		stream.write((char*)&convex, sizeof(convex));
		
		std::vector<PxVec3> vertices(vertex_count);
		for (int i = 0; i < vertex_count; i++)
			vertices[i] = Physics::Convert(submesh.vertices[i].position);

		if (is_convex)
		{
			PxConvexMeshDesc convexDesc;
			convexDesc.points.count = vertex_count;
			convexDesc.points.stride = sizeof(PxVec3);
			convexDesc.points.data = vertices.data();
			convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

			PxDefaultMemoryOutputStream buf;
			if (!cooking->cookConvexMesh(convexDesc, buf))
				throw std::runtime_error("cooking failed");

			uint32_t size = buf.getSize();
			stream.write((char*)&size, sizeof(size));
			stream.write((char*)buf.getData(), size);
		}
		else
		{
			PxTriangleMeshDesc meshDesc;
			meshDesc.points.count = vertex_count;
			meshDesc.points.stride = sizeof(PxVec3);
			meshDesc.points.data = vertices.data();

			meshDesc.triangles.count = triangle_count;
			meshDesc.triangles.stride = 3 * sizeof(PxU32);
			meshDesc.triangles.data = submesh.indices.data();

			PxDefaultMemoryOutputStream buf;
			if (!cooking->cookTriangleMesh(meshDesc, buf))
				throw std::runtime_error("cooking failed");

			uint32_t size = buf.getSize();
			stream.write((char*)&size, sizeof(size));
			stream.write((char*)buf.getData(), size);
		}
	}

	bool WritePhysMeshToFile(const std::vector<SubMesh>& meshes, const std::wstring& filename, bool is_convex, PxCooking* cooking)
	{
		fs::path path = filename;
		fs::create_directories(path.parent_path());

		const uint32_t sub_mesh_count = meshes.size();

		std::ofstream stream(filename, std::ios::binary);
		stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		stream.write((char*)&PHYS_FILE_MAGIC, sizeof(PHYS_FILE_MAGIC));
		stream.write((char*)&PHYS_FILE_VERSION, sizeof(PHYS_FILE_VERSION));
		stream.write((char*)&sub_mesh_count, sizeof(sub_mesh_count));

		for (auto& mesh : meshes)
			WritePhysMesh(stream, mesh, is_convex, cooking);

		return stream.good();
	}
}