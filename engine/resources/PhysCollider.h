#pragma once

#include "ResourceCache.h"
#include "render/device/Resource.h"
#include <string>
#include "utils/Math.h"
#include "scene/Physics.h"

namespace Resources
{

	class PhysCollider
	{
	public:
		using Handle = Handle<PhysCollider>;

		PhysCollider(const std::wstring& filename);
		~PhysCollider();

		const size_t GetMeshCount() const { return mesh_colliders.size(); }
		const auto* GetMesh(uint32_t index) const { assert(index < mesh_colliders.size()); return mesh_colliders[index].get(); }
		const size_t GetConvexCount() const { return convex_colliders.size(); }
		physx::PxConvexMesh* GetConvex(uint32_t index) const { assert(index < convex_colliders.size()); return convex_colliders[index].get(); }

	private:
		void LoadPhysMesh(std::istream& stream);

	private:
		mutable std::vector<Physics::Handle<physx::PxConvexMesh>> convex_colliders;
		mutable std::vector<Physics::Handle<physx::PxTriangleMesh>> mesh_colliders;
	};

}