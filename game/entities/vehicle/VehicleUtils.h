#pragma once

#include "scene/Physics.h"
#include "ecs/ECS.h"
#include "SceneQuery.h"

namespace Vehicle::Utils
{
	enum
	{
		COLLISION_FLAG_GROUND			=	1 << 0,
		COLLISION_FLAG_WHEEL			=	1 << 1,
		COLLISION_FLAG_CHASSIS			=	1 << 2,
		COLLISION_FLAG_OBSTACLE			=	1 << 3,
		COLLISION_FLAG_DRIVABLE_OBSTACLE=	1 << 4,

		COLLISION_FLAG_GROUND_AGAINST	=															COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE | COLLISION_FLAG_DRIVABLE_OBSTACLE,
		COLLISION_FLAG_WHEEL_AGAINST	=									COLLISION_FLAG_WHEEL |	COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE,
		COLLISION_FLAG_CHASSIS_AGAINST	=			COLLISION_FLAG_GROUND | COLLISION_FLAG_WHEEL |	COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE | COLLISION_FLAG_DRIVABLE_OBSTACLE,
		COLLISION_FLAG_OBSTACLE_AGAINST	=			COLLISION_FLAG_GROUND | COLLISION_FLAG_WHEEL |	COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE | COLLISION_FLAG_DRIVABLE_OBSTACLE,
		COLLISION_FLAG_DRIVABLE_OBSTACLE_AGAINST=	COLLISION_FLAG_GROUND 						 |	COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE | COLLISION_FLAG_DRIVABLE_OBSTACLE
	};

	//Drivable surface types.
	enum
	{
		SURFACE_TYPE_TARMAC,
		MAX_NUM_SURFACE_TYPES
	};

	//Tire types.
	enum
	{
		TIRE_TYPE_NORMAL = 0,
		TIRE_TYPE_WORN,
		MAX_NUM_TIRE_TYPES
	};

	physx::PxFilterFlags VehicleFilterShader
	(physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0, 
		physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
		physx::PxPairFlags& pairFlags, const void* constantBlock, physx::PxU32 constantBlockSize);

	struct VehicleDesc
	{
		VehicleDesc()
			: chassisMass(0.0f),
			chassisDims(physx::PxVec3(0.0f, 0.0f, 0.0f)),
			chassisMOI(physx::PxVec3(0.0f, 0.0f, 0.0f)),
			chassisCMOffset(physx::PxVec3(0.0f, 0.0f, 0.0f)),
			chassisMaterial(NULL),
			wheelMass(0.0f),
			wheelWidth(0.0f),
			wheelRadius(0.0f),
			wheelMOI(0.0f),
			wheelMaterial(NULL)
		{
		}

		physx::PxF32 chassisMass;
		physx::PxVec3 chassisDims;
		physx::PxVec3 chassisMOI;
		physx::PxVec3 chassisCMOffset;
		physx::PxMaterial* chassisMaterial;
		physx::PxFilterData chassisSimFilterData;  //word0 = collide type, word1 = collide against types, word2 = PxPairFlags

		physx::PxF32 wheelMass;
		physx::PxF32 wheelWidth;
		physx::PxF32 wheelRadius;
		physx::PxF32 wheelMOI;
		physx::PxMaterial* wheelMaterial;
		physx::PxU32 numWheels;
		physx::PxFilterData wheelSimFilterData;	//word0 = collide type, word1 = collide against types, word2 = PxPairFlags
	};

	class VehicleDataCache
	{
		VehicleSceneQueryData* scene_query = nullptr;
		Physics::Handle<physx::PxBatchQuery> batch_query;
		Physics::Handle<physx::PxVehicleDrivableSurfaceToTireFrictionPairs> friction_pairs;
		physx::PxAllocatorCallback& allocator;
	public:
		VehicleDataCache(physx::PxScene& scene, physx::PxAllocatorCallback& allocator, physx::PxMaterial& material);
		~VehicleDataCache();

		VehicleSceneQueryData* GetSceneQueryData() { return scene_query; };
		physx::PxBatchQuery* GetBatchQuery() { return batch_query.get(); };
		physx::PxVehicleDrivableSurfaceToTireFrictionPairs* GetFrictionPairs() { return friction_pairs.get(); };
	};

	VehicleDesc InitVehicleDesc(physx::PxMaterial* material);
	ECS::EntityID CreateDrivablePlane(ECS::EntityManager& manager, Physics::PhysXManager& physics, const physx::PxFilterData& simFilterData, physx::PxMaterial* material);
	ECS::EntityID CreateVehicle(ECS::EntityManager& manager, Physics::PhysXManager& physics, vec3 position, quat rotation, VehicleDesc& desk);

}