#include <fstream>
#include <limits>

#include "Engine.h"
#include "scene/Scene.h"
#include "PhysCollider.h"

namespace Resources
{
	PhysCollider::PhysCollider(const std::wstring& filename)
	{
		std::ifstream stream(filename, std::ios::binary);
		stream.exceptions(std::ios::badbit | std::ios::failbit);

		uint32_t magic, version, mesh_count;
		stream.read((char*)&magic, sizeof(magic));

		if (magic != 'phys')
			throw std::runtime_error("Not a phys mesh");

		stream.read((char*)&version, sizeof(version));
		stream.read((char*)&mesh_count, sizeof(mesh_count));

		for (int i = 0; i < mesh_count; i++)
		{
			LoadPhysMesh(stream);
		}
	}

	void PhysCollider::LoadPhysMesh(std::istream& stream)
	{
		uint8_t convex;
		uint32_t size;

		stream.read((char*)&convex, sizeof(convex));
		stream.read((char*)&size, sizeof(size));

		if (!size)
			throw std::runtime_error("phys mesh can't be zero size");

		std::vector<uint8_t> data;
		data.resize(size);
		stream.read((char*)data.data(), size);

		physx::PxDefaultMemoryInputData read_buffer(data.data(), data.size());
		
		auto physics = Engine::Get()->GetScene()->GetPhysics()->GetPhysX();
		if (convex)
		{
			auto mesh = Physics::Handle(physics->createConvexMesh(read_buffer));
			convex_colliders.push_back(std::move(mesh));
		}
		else
		{
			auto mesh = Physics::Handle(physics->createTriangleMesh(read_buffer));
			mesh_colliders.push_back(std::move(mesh));
		}
	}

	PhysCollider::~PhysCollider() = default;	
}
