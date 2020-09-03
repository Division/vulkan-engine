#include "Physics.h"

using namespace physx;

namespace Physics
{

	void* PhysXManager::Allocator::allocate(size_t size, const char* typeName, const char* filename, int line)
	{
		void* ptr = allocator.allocate(size);

		std::scoped_lock lock(mutex);
		allocations[ptr] = size;

		return ptr;
	}

	void PhysXManager::Allocator::deallocate(void* ptr)
	{
		size_t size;

		{
			std::scoped_lock lock(mutex);
			auto it = allocations.find(ptr);
			if (it == allocations.end())
				throw std::runtime_error("Invalid deallocation");

			size = it->second;
			allocations.erase(it);
		}

		allocator.deallocate((uint8_t*)ptr, size);
	}

	void PhysXManager::ErrorCallback::reportError(PxErrorCode::Enum code, const char* message, const char* file, int line)
	{
		std::cout << "[PhysX Error] " << message << "\n" << file << line << "\n";
	}

	PhysXManager::PhysXManager()
	{
		foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, error_callback);
		if(!foundation)
			throw std::runtime_error("PxCreateFoundation failed!");

		if (use_remote_debugger)
		{
			debugger = PxCreatePvd(*foundation);
			transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
			debugger->connect(*transport, PxPvdInstrumentationFlag::eALL);
		}

		
		PxTolerancesScale scale;
		cooking = PxCreateCooking(PX_PHYSICS_VERSION, *foundation, PxCookingParams(scale));
		if (!cooking)
			throw std::runtime_error("PxCreateCooking failed!");

		physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, scale, true);
		if (!physics)
			throw std::runtime_error("PxCreatePhysics failed!");

		PxSceneDesc scene_desc(physics->getTolerancesScale());
		scene_desc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
		dispatcher = PxDefaultCpuDispatcherCreate(std::thread::hardware_concurrency());
		scene_desc.cpuDispatcher = dispatcher.get();
		scene_desc.filterShader	= PxDefaultSimulationFilterShader;
		default_scene = physics->createScene(scene_desc);
		
		auto pvd_client = default_scene->getScenePvdClient();
		if(pvd_client)
		{
			pvd_client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
			pvd_client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
			pvd_client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
		}
	}

	void PhysXManager::StepPhysics(float dt)
	{
		default_scene->simulate(1.0f/60.0f);
	}

	void PhysXManager::FetchResults()
	{
		default_scene->fetchResults(true);
	}

	PhysXManager::~PhysXManager()
	{

	}


}