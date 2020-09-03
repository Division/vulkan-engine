#pragma once

#if defined(_DEBUG)
#pragma message("WARNING: Building with _DEBUG")
#endif

#if defined(NDEBUG)

#pragma message("WARNING: Building with NDEBUG")
#endif

#pragma message("WARNING: asdasd")

#include <unordered_map>
#include <mutex>
#include <physx/PxPhysicsAPI.h>
#include "memory/Allocator.h"
#include <memory>

namespace Physics
{
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
		PhysXManager();
		~PhysXManager();

		void StepPhysics(float dt);
		void FetchResults();

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
		
		// Order is important
		Handle<physx::PxFoundation> foundation;
		Handle<physx::PxPhysics> physics;
		Handle<physx::PxCooking> cooking;
		Handle<physx::PxPvd> debugger;
		Handle<physx::PxPvdTransport> transport;
		Handle<physx::PxDefaultCpuDispatcher> dispatcher;
		Handle<physx::PxScene> default_scene;
		bool use_remote_debugger = true;
	};

}