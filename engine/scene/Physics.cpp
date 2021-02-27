#include "Physics.h"
#include "Engine.h"
#include "render/debug/DebugDraw.h"
#include "ecs/components/Static.h"

using namespace physx;

namespace Physics
{

	constexpr bool DEBUG_RENDER_ENABLED_AT_START = false;

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

	PhysXManager::PhysXManager(IGamePhysicsDelegate* delegate, ECS::components::DeltaTime* delta_time)
		: delegate(delegate)
		, delta_time(delta_time)
	{
		foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, error_callback);
		if(!foundation)
			throw std::runtime_error("PxCreateFoundation failed!");

		if (use_remote_debugger)
		{
			debugger = PxCreatePvd(*foundation);
			transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
			if (!debugger->connect(*transport, PxPvdInstrumentationFlag::eALL))
			{
				std::wcout << "Couldn't connect physx visual debugger\n";
			}
		}
		
		PxTolerancesScale scale;
		cooking = PxCreateCooking(PX_PHYSICS_VERSION, *foundation, PxCookingParams(scale));
		if (!cooking)
			throw std::runtime_error("PxCreateCooking failed!");

		physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, scale, true, debugger.get());
		if (!physics)
			throw std::runtime_error("PxCreatePhysics failed!");

		if (!PxInitVehicleSDK(*physics))
			throw std::runtime_error("PxInitVehicleSDK failed!");

		PxVehicleSetBasisVectors(PxVec3(0, 1, 0), PxVec3(0, 0, -1));
		PxVehicleSetUpdateMode(PxVehicleUpdateMode::eACCELERATION);

		PxSceneDesc scene_desc(physics->getTolerancesScale());
		scene_desc.gravity = delegate->GetGravity();
		dispatcher = PxDefaultCpuDispatcherCreate(std::thread::hardware_concurrency());
		scene_desc.cpuDispatcher = dispatcher.get();
		scene_desc.filterShader	= delegate ? delegate->GetFilterShader() : PxDefaultSimulationFilterShader;
		default_scene = physics->createScene(scene_desc);

		auto pvd_client = default_scene->getScenePvdClient();
		if(pvd_client)
		{
			pvd_client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
			pvd_client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
			pvd_client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
		}

		default_material = physics->createMaterial(0.5f, 0.5f, 0.6f);

		if constexpr (DEBUG_RENDER_ENABLED_AT_START)
		{
			SetDebugRenderEnabled(true);
		}

	}

	void PhysXManager::StepPhysics(float dt)
	{
		current_time += dt;
		const auto time_since_update = current_time - last_time;
		if (time_since_update > 0.5)
			last_time = current_time - 0.5;

		const float max_dt = 1.0f / 40.0f;
		const float update_interval = 1.0f / 60.0f;

		if (time_since_update >= update_interval)
		{
			// Allowing higher DT to compensate longer frames. May worth trying multiple fixed steps as well.
			const auto simulation_dt = std::min((float)time_since_update, max_dt);

			if (delta_time)
				delta_time->physics_dt = simulation_dt;

			if (delegate)
				delegate->UpdatePhysics(simulation_dt);

			default_scene->simulate(simulation_dt);
			last_time += simulation_dt;
			has_results = true;
		}
	}

	void PhysXManager::FetchResults()
	{
		if (!has_results) return;
		default_scene->fetchResults(true);
		has_results = false;
	}

	void PhysXManager::SetDebugRenderEnabled(bool enabled)
	{
		if (enabled)
		{
			default_scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
			default_scene->setVisualizationParameter(PxVisualizationParameter::eACTOR_AXES, 1.0f);
			default_scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);
			default_scene->setVisualizationParameter(PxVisualizationParameter::eCONTACT_POINT, 1.0f);
		}
		else
		{
			default_scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 0.0f);
		}
	}

	bool PhysXManager::GetDebugRenderEnabled()
	{
		return default_scene->getVisualizationParameter(PxVisualizationParameter::eSCALE) > 0.001f;
	}

	void PhysXManager::DrawDebug()
	{
		if (!GetDebugRenderEnabled())
			return;

		const PxRenderBuffer& rb = default_scene->getRenderBuffer();
		auto* debug_draw = Engine::Get()->GetDebugDraw();

		for(uint32_t i = 0; i < rb.getNbLines(); i++)
		{
			const PxDebugLine& line = rb.getLines()[i];
			auto color = RGBAFromUint(line.color0);
			color.a = 1.0f;
			debug_draw->DrawLine(vec3(line.pos0.x, line.pos0.y, line.pos0.z), vec3(line.pos1.x, line.pos1.y, line.pos1.z), color);
		}

		for(uint32_t i = 0; i < rb.getNbTriangles(); i++)
		{
			const auto& triangle = rb.getTriangles()[i];
			auto color = RGBAFromUint(triangle.color0);
			color.a = 1.0f;
			debug_draw->DrawLine(vec3(triangle.pos0.x, triangle.pos0.y, triangle.pos0.z), vec3(triangle.pos1.x, triangle.pos1.y, triangle.pos1.z), color);
			debug_draw->DrawLine(vec3(triangle.pos1.x, triangle.pos1.y, triangle.pos1.z), vec3(triangle.pos2.x, triangle.pos2.y, triangle.pos2.z), color);
			debug_draw->DrawLine(vec3(triangle.pos2.x, triangle.pos2.y, triangle.pos2.z), vec3(triangle.pos0.x, triangle.pos0.y, triangle.pos0.z), color);
		}

		for(uint32_t i = 0; i < rb.getNbPoints(); i++)
		{
			const auto& point = rb.getPoints()[i];
			auto color = RGBAFromUint(point.color);
			color.a = 1.0f;
			debug_draw->DrawPoint(vec3(point.pos.x, point.pos.y, point.pos.z), color, 5.0f);
		}
	}

	PhysXManager::~PhysXManager()
	{
		PxCloseVehicleSDK();
	}


	// Utils

	Handle<PxRigidDynamic> PhysXManager::CreateDynamic(const vec3 position, const quat rotation, const PxGeometry& geometry, PxMaterial* material, bool add_to_scene)
	{
		if (!material)
			material = default_material.get();

		PxTransform transform = ConvertTransform(position, rotation);
		PxRigidDynamic* dynamic = PxCreateDynamic(*physics, transform, geometry, *material, 10.0f);
		dynamic->setAngularDamping(0.5f);

		if (add_to_scene)
			default_scene->addActor(*dynamic);

		return dynamic;
	}

	Handle<PxRigidStatic> PhysXManager::CreateStatic(const vec3 position, const quat rotation, const PxGeometry& geometry, PxMaterial* material, bool add_to_scene)
	{
		if (!material)
			material = default_material.get();

		PxTransform transform = ConvertTransform(position, rotation);
		auto body = Handle(PxCreateStatic(*physics, transform, geometry, *material));

		if (add_to_scene)
			default_scene->addActor(*body);

		return body;
	}

	std::vector<Handle<PxRigidDynamic>> PhysXManager::CreateStack(const vec3 position, const quat rotation, uint32_t size, float halfExtent, PxMaterial* material)
	{
		if (!material)
			material = default_material.get();

		PxTransform t = ConvertTransform(position, rotation);

		std::vector<Handle<PxRigidDynamic>> result;

		auto shape = Handle(physics->createShape(PxBoxGeometry(halfExtent, halfExtent, halfExtent), *material));
		
		for(PxU32 i=0; i<size;i++)
		{
			for(PxU32 j=0;j<size-i;j++)
			{
				PxTransform localTm(PxVec3(PxReal(j*2) - PxReal(size-i), PxReal(i*2+1), 0) * halfExtent);
				auto body = Handle(physics->createRigidDynamic(t.transform(localTm)));
				body->attachShape(*shape);
				PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
				default_scene->addActor(*body);
				result.push_back(std::move(body));
			}
		}

		return result;
	}

	Handle<physx::PxRigidStatic> PhysXManager::CreatePlaneStatic(const vec3 position, const quat rotation, physx::PxMaterial* material)
	{
		return CreateStatic(position, rotation, PxPlaneGeometry());
	}

	Handle<physx::PxRigidDynamic> PhysXManager::CreateBoxDynamic(const vec3 position, const quat rotation, float half_size, physx::PxMaterial* material)
	{
		return CreateDynamic(position, rotation, PxBoxGeometry(half_size, half_size, half_size));
	}

	Handle<physx::PxRigidDynamic> PhysXManager::CreateSphereDynamic(const vec3 position, const quat rotation, float radius, physx::PxMaterial* material)
	{
		return CreateDynamic(position, rotation, PxSphereGeometry(radius));
	}

	Handle<physx::PxRigidStatic> PhysXManager::CreateSphereStatic(const vec3 position, const quat rotation, float radius, physx::PxMaterial* material)
	{
		return CreateStatic(position, rotation, PxSphereGeometry(radius));
	}

}
