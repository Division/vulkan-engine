#include <unordered_map>
#include <filesystem>
#include <fstream>
#include "MeshExporter.h"
#include "scene/Physics.h"
#include "FBXUtils.h"
#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/offline/fbx/fbx.h"
#include "ozz/base/containers/map.h"
#include <numeric>
#include <map>

namespace fs = std::filesystem;
using namespace physx;
using namespace fbxsdk;

namespace Exporter
{
	bool MeshVertex::operator==(const MeshVertex& other) const
	{
		return std::tie(position, normal, binormal, tangent, uv0, weights, bone_indices) ==
			std::tie(other.position, other.normal, other.binormal, other.tangent, other.uv0, other.weights, other.bone_indices);
	}

	uint32_t MeshVertex::GetHash() const
	{
		uint8_t data[sizeof(position) + sizeof(normal) + sizeof(binormal) + sizeof(tangent) + sizeof(uv0) + sizeof(weights) + sizeof(bone_indices)];
		uint8_t* ptr = data;
		memcpy(ptr, &position, sizeof(position)); ptr += sizeof(position);
		memcpy(ptr, &normal, sizeof(normal)); ptr += sizeof(normal);
		memcpy(ptr, &binormal, sizeof(binormal)); ptr += sizeof(binormal);
		memcpy(ptr, &tangent, sizeof(tangent)); ptr += sizeof(tangent);
		memcpy(ptr, &uv0, sizeof(uv0)); ptr += sizeof(uv0);
		memcpy(ptr, &weights, sizeof(weights)); ptr += sizeof(weights);
		memcpy(ptr, &bone_indices, sizeof(bone_indices)); ptr += sizeof(bone_indices);

		return FastHash(data, std::size(data));
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

	struct BoneWeight
	{
		uint16_t index; // bone index
		float weight;
		bool operator<(const BoneWeight& other) const { return weight < other.weight; }
		bool operator>(const BoneWeight& other) const { return weight > other.weight; }
	};

	struct VertexSkinningData
	{
		MeshVertex* vertex;
		std::vector<BoneWeight> weights;
	};

	void AddSkinningWeights(FbxScene* scene, SubMesh& sub_mesh, FbxMesh *mesh, FbxSkin* deformer, ozz::animation::Skeleton* skeleton, std::vector<MeshTriangle>& triangles)
	{
		std::vector<std::vector<VertexSkinningData>> skinning_data;
		skinning_data.resize(mesh->GetControlPointsCount());

		ozz::animation::offline::fbx::FbxSystemConverter converter(scene->GetGlobalSettings().GetAxisSystem(), scene->GetGlobalSettings().GetSystemUnit());

		std::map<uint16_t, uint16_t> joint_remap;

		sub_mesh.inv_bindpose.resize(skeleton->num_joints(), glm::mat4());

		// Builds joints names map.
		ozz::cstring_map<uint16_t> joint_name_map;
		for (int i = 0; i < skeleton->num_joints(); ++i) {
			joint_name_map[skeleton->joint_names()[i]] = static_cast<uint16_t>(i);
		}

		for (auto& triangle : triangles)
			for (auto& vertex : triangle.vertices)
			{
				if (vertex.control_point_index >= skinning_data.size())
					throw std::runtime_error("control point index exceeds estimated limit");
				auto& item = skinning_data[vertex.control_point_index].emplace_back();
				item.vertex = &vertex;
			}

		// Computes geometry matrix.
		const FbxAMatrix geometry_matrix(mesh->GetNode()->GetGeometricTranslation(FbxNode::eSourcePivot), mesh->GetNode()->GetGeometricRotation(FbxNode::eSourcePivot), mesh->GetNode()->GetGeometricScaling(FbxNode::eSourcePivot));

		// Collect all joint weights from the clusters
		const int cluster_count = deformer->GetClusterCount();
		for (int cl = 0; cl < cluster_count; ++cl) 
		{
			FbxCluster* cluster = deformer->GetCluster(cl);
			FbxNode* node = cluster->GetLink();
			if (!node) continue;

			const FbxCluster::ELinkMode mode = cluster->GetLinkMode();
			if (mode != FbxCluster::eNormalize)
				throw std::runtime_error(std::string("Unsupported link mode for joint ") + node->GetName());

			// Get corresponding joint index;
			auto it = joint_name_map.find(node->GetName());
			if (it == joint_name_map.end())
				throw std::runtime_error(std::string("Required joint ") + node->GetName() + " not found in provided skeleton.\n");

			const uint16_t joint = it->second;

			joint_remap.insert({ joint, 0 }); // fill the remap list with all joints used by the submesh

			// Computes joint's inverse bind-pose matrix.
			FbxAMatrix transform_matrix;
			cluster->GetTransformMatrix(transform_matrix);
			transform_matrix *= geometry_matrix;

			FbxAMatrix transform_link_matrix;
			cluster->GetTransformLinkMatrix(transform_link_matrix);

			const FbxAMatrix inverse_bind_pose = transform_link_matrix.Inverse() * transform_matrix;

			// Stores inverse transformation.
			sub_mesh.inv_bindpose[joint] = (mat4&)converter.ConvertMatrix(inverse_bind_pose);

			// Affect joint to all vertices of the cluster.
			const int ctrl_point_index_count = cluster->GetControlPointIndicesCount();

			const int* ctrl_point_indices = cluster->GetControlPointIndices();
			const double* ctrl_point_weights = cluster->GetControlPointWeights();
			for (int cpi = 0; cpi < ctrl_point_index_count; ++cpi) 
			{
				const BoneWeight bone_weight = { joint, static_cast<float>(ctrl_point_weights[cpi]) };
				if (bone_weight.weight < 1e-5) continue;

				const int ctrl_point = ctrl_point_indices[cpi];
				assert(ctrl_point < static_cast<int>(skinning_data.size()));

				for (auto& vertices : skinning_data[ctrl_point])
					vertices.weights.push_back(bone_weight);

			}
		}

		// Sub mesh may use only part of the joints so need to map sub mesh joint index to full skeleton joint index
		int index = 0;
		sub_mesh.bone_index_remap.resize(joint_remap.size());
		for (auto& pair : joint_remap)
		{
			sub_mesh.bone_index_remap[index] = pair.first;
			pair.second = index;
			index++;
		}

		for (auto& vertices : skinning_data)
			for (auto& vertex : vertices)
			{
				std::sort(vertex.weights.begin(), vertex.weights.end(), std::greater<BoneWeight>());
				vertex.weights.resize(std::min((size_t)4, vertex.weights.size()));
				if (vertex.weights.empty())
					throw std::runtime_error("No bones assigned to a vertex");

				vertex.vertex->weights = vec4(0, 0, 0, 0);
				vertex.vertex->bone_indices = ivec4(0, 0, 0, 0);
				float sum = 0;
				for (int i = 0; i < vertex.weights.size(); i++)
				{
					vertex.vertex->weights[i] = vertex.weights[i].weight;
					vertex.vertex->bone_indices[i] = joint_remap.at(vertex.weights[i].index); // Assigning remapped index
					sum += vertex.weights[i].weight;
				}
				
				if (sum > 1e-3)
					vertex.vertex->weights /= sum; // normalize weights
			}
	}

	std::vector<SubMesh> ExtractMeshes(const std::vector<SourceMesh>& meshes, FbxNode* parent_node, FbxScene* scene, ozz::animation::Skeleton* skeleton)
	{
		ozz::animation::offline::fbx::FbxSystemConverter converter(scene->GetGlobalSettings().GetAxisSystem(), scene->GetGlobalSettings().GetSystemUnit());

		std::vector<SubMesh> result;
		std::vector<FbxVector4> control_points;

		// Maps material id to triangle list
		std::unordered_map<FbxGeometryElementMaterial*, std::vector<MeshTriangle>> material_triangles;

		auto root_node_inv_transform = parent_node ? parent_node->EvaluateGlobalTransform() : fbxsdk::FbxAMatrix();
		root_node_inv_transform = fbxsdk::FbxAMatrix(-root_node_inv_transform.GetT(), FbxVector4(), FbxVector4(1,1,1,1)); // Inverse translation only

		for (auto& mesh_pair : meshes)
		{
			auto* mesh = mesh_pair.second;

			const bool lHasVertexCache = mesh->GetDeformerCount(FbxDeformer::eVertexCache) &&
				(static_cast<FbxVertexCacheDeformer*>(mesh->GetDeformer(0, FbxDeformer::eVertexCache)))->Active.Get();
			const bool lHasShape = mesh->GetShapeCount() > 0;


			control_points.resize(mesh->GetControlPointsCount());
			memcpy(control_points.data(), &mesh->GetControlPoints()[0], sizeof(FbxVector4) * mesh->GetControlPointsCount());

			FbxAMatrix identity; identity.SetIdentity();
			//ComputeLinearDeformation(identity, mesh, FbxTime(), control_points.data(), nullptr);

			// Transformation related to root node
			auto mesh_transform = root_node_inv_transform * mesh_pair.first->EvaluateGlobalTransform();

			if (!mesh->IsTriangleMesh())
				throw std::runtime_error("Mesh has non-triangle polygons: " + std::string(mesh->GetName()));

			auto* material = mesh->GetElementMaterial();
			auto& triangles = material_triangles[material];
			auto& sub_mesh = result.emplace_back();

			const int skin_count = mesh->GetDeformerCount(FbxDeformer::eSkin);
			if (skin_count > 1)
				throw std::runtime_error("Mesh has more than one skin: " + std::string(mesh->GetName()));

			sub_mesh.name = mesh->GetName();
			sub_mesh.has_normals = mesh->GetElementNormalCount() > 0;
			sub_mesh.has_binormals = mesh->GetElementBinormalCount() > 0;
			sub_mesh.has_tangents = mesh->GetElementTangentCount() > 0;
			sub_mesh.has_skinning_weights = skin_count == 1;
			sub_mesh.has_uv0 = mesh->GetElementUVCount() > 0;

			/*if (sub_mesh.has_skinning_weights ^ (bool)skeleton)
				throw std::runtime_error("Skeleton not provided mismatch: " + std::string(mesh->GetName()));*/

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
					FbxVector4 fbx_vertex = control_points[index];
					fbx_vertex = mesh_transform.MultT(fbx_vertex);
					triangle.vertices[v].position = (vec3&)converter.ConvertPoint(fbx_vertex);
					triangle.vertices[v].control_point_index = index;
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
						triangle.vertices[v].uv0 = vec2(fbx_uv[0], 1.0f - fbx_uv[1]);
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

			if (sub_mesh.has_skinning_weights)
			{
				auto deformer = static_cast<FbxSkin*>(mesh->GetDeformer(0, FbxDeformer::eSkin));
				FbxSkin::EType skinning_type = deformer->GetSkinningType();
				if (skinning_type != FbxSkin::eRigid && skinning_type != FbxSkin::eLinear)
					throw std::runtime_error("Unsupported skinning type");
				
				AddSkinningWeights(scene, sub_mesh, mesh, deformer, skeleton, triangles);
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
			Vector2Half uv(vertex.uv0.x, vertex.uv0.y);
			stream.write((char*)&uv, sizeof(uv));
		}

		if (submesh.has_skinning_weights)
		{
			Vector4b weights = Vector4b::FromNormalizedFloat(vertex.weights);
			Vector4b indices = Vector4b::FromUInt(vertex.bone_indices);
			stream.write((char*)&indices, sizeof(indices));
			stream.write((char*)&weights, sizeof(weights));
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
		const uint8_t name_length = std::min((size_t)std::numeric_limits<uint8_t>::max(), submesh.name.size());

		stream.write((char*)&flags, sizeof(flags));
		stream.write((char*)&vertex_count, sizeof(vertex_count));
		stream.write((char*)&triangle_count, sizeof(triangle_count));
		stream.write((char*)&submesh.aabb, sizeof(submesh.aabb));
		stream.write((char*)&name_length, sizeof(name_length));
		stream.write(submesh.name.data(), name_length);

		if (submesh.has_skinning_weights)
		{
			auto bone_count = (uint16_t)submesh.bone_index_remap.size();
			stream.write((char*)&bone_count, sizeof(bone_count));
			stream.write((char*)submesh.bone_index_remap.data(), bone_count * sizeof(uint16_t));
			for (int i = 0; i < bone_count; i++)
				stream.write((char*)&submesh.inv_bindpose[submesh.bone_index_remap[i]], sizeof(mat4));
		}

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
		if (meshes.empty())
		{
			std::cout << "mesh list is empty\n";
			return false;
		}

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