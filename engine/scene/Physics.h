#pragma once

#include <unordered_map>
#include <mutex>
#include <physx/PxPhysicsAPI.h>
#include "memory/Allocator.h"
#include <memory>
#include "utils/Math.h"
#include <functional>

class IGamePhysicsDelegate;

namespace ECS::components
{
	struct DeltaTime;
}

namespace Physics
{
	inline vec3 Convert(physx::PxVec3 v) { return vec3(v.x, v.y, v.z); }
	inline physx::PxVec3 Convert(vec3 v) { return physx::PxVec3(v.x, v.y, v.z); }
	inline quat Convert(physx::PxQuat q) { return quat(q.w, q.x, q.y, q.z); }
	inline physx::PxQuat Convert(quat q) { return physx::PxQuat(q.x, q.y, q.z, q.w); }
	inline physx::PxTransform ConvertTransform(vec3 position, quat rotation) { return physx::PxTransform(Convert(position), Convert(rotation)); }
	inline void ConvertTransform(const physx::PxTransform& transform, vec3& out_position, quat& out_rotation) { out_position = Convert(transform.p); out_rotation = Convert(transform.q); }

	template<typename T>
	class Handle
	{
	public:
		Handle() : pointer(nullptr){}
		Handle(T* pointer) : pointer(pointer) {}
		Handle(Handle&& other)
		{
			pointer = other.pointer;
			other.pointer = nullptr;
		}
		Handle(const Handle&) = delete;

		Handle& operator=(Handle&& other)
		{
			if (pointer)
				pointer->release();

			pointer = other.pointer;
			other.pointer = nullptr;
			return *this;
		}

		Handle& operator=(nullptr_t)
		{
			if (pointer)
				pointer->release();

			pointer = nullptr;
			return *this;
		}

		operator bool() const { return (bool)pointer; }

		T* operator->() { return pointer; }
		const T* operator->() const { return pointer; }
		T& operator*() { return *pointer; }
		const T& operator*() const { return *pointer; }
		T* get() { return pointer; }
		const T* get() const { return pointer; }

		~Handle()
		{
			if (pointer)
				pointer->release();
		}

		void reset()
		{
			if (pointer)
				pointer->release();
			
			pointer = nullptr;
		}

	private:
		T* pointer;
	};


	class PhysXManager
	{
	public:
		PhysXManager(IGamePhysicsDelegate* delegate, ECS::components::DeltaTime* delta_time);
		~PhysXManager();

		void StepPhysics(float dt);
		void FetchResults();
		void SetDebugRenderEnabled(bool enabled);
		bool GetDebugRenderEnabled();
		void DrawDebug();
		physx::PxPhysics* GetPhysX() { return physics.get(); };
		physx::PxCooking* GetCooking() { return cooking.get(); };
		physx::PxFoundation* GetFoundation() { return foundation.get(); };
		physx::PxScene* GetScene() { return default_scene.get(); };
		physx::PxAllocatorCallback* GetAllocator() { return &allocator; };
		// Utility functions
		Handle<physx::PxRigidDynamic> CreateDynamic(const vec3 position, const quat rotation, const physx::PxGeometry& geometry, physx::PxMaterial* material = nullptr, bool add_to_scene = true);
		Handle<physx::PxRigidStatic> CreateStatic(const vec3 position, const quat rotation, const physx::PxGeometry& geometry, physx::PxMaterial* material = nullptr, bool add_to_scene = true);
		std::vector<Handle<physx::PxRigidDynamic>> CreateStack(const vec3 position, const quat rotation, uint32_t size, float halfExtent, physx::PxMaterial* material = nullptr);
		Handle<physx::PxRigidStatic> CreatePlaneStatic(const vec3 position, const quat rotation, physx::PxMaterial* material = nullptr);
		Handle<physx::PxRigidDynamic> CreateBoxDynamic(const vec3 position, const quat rotation, float half_size, physx::PxMaterial* material = nullptr);
		Handle<physx::PxRigidDynamic> CreateSphereDynamic(const vec3 position, const quat rotation, float readius, physx::PxMaterial* material = nullptr);
		Handle<physx::PxRigidStatic> CreateSphereStatic(const vec3 position, const quat rotation, float radius, physx::PxMaterial* material = nullptr);

	private:
		class Allocator : public physx::PxAllocatorCallback
		{
			Memory::TaggedAllocator<uint8_t, Memory::Tag::Physics> allocator;
			std::unordered_map<void*, size_t> allocations;
			std::mutex mutex;
		public:
			void* allocate(size_t size, const char* typeName, const char* filename, int line) override;
			void deallocate(void* ptr) override;
		};

		class ErrorCallback : public physx::PxErrorCallback
		{
		public:
			void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line) override;
		};

		Allocator allocator;
		ErrorCallback error_callback;
		IGamePhysicsDelegate* delegate;
		ECS::components::DeltaTime* delta_time;

		// Order is important for correct deinitialization
		Handle<physx::PxFoundation> foundation;
		Handle<physx::PxPvdTransport> transport;
		Handle<physx::PxPvd> debugger;
		Handle<physx::PxPhysics> physics;
		Handle<physx::PxCooking> cooking;
		Handle<physx::PxDefaultCpuDispatcher> dispatcher;
		Handle<physx::PxScene> default_scene;
		Handle<physx::PxMaterial> default_material;
		bool use_remote_debugger = true;
		double last_time = 0.0f;
		double current_time = 0.0f;
		bool has_results = false;
	};

}