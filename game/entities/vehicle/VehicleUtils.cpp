#include "VehicleUtils.h"
#include "utils/Math.h"
#include "ecs/components/Physics.h"
#include "ecs/components/Transform.h"

using namespace physx;
using namespace Physics;

namespace Vehicle::Utils
{
	PxFilterFlags VehicleFilterShader
	(PxFilterObjectAttributes attributes0, PxFilterData filterData0, 
		PxFilterObjectAttributes attributes1, PxFilterData filterData1,
		PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
	{
		PX_UNUSED(attributes0);
		PX_UNUSED(attributes1);
		PX_UNUSED(constantBlock);
		PX_UNUSED(constantBlockSize);

		if( (0 == (filterData0.word0 & filterData1.word1)) && (0 == (filterData1.word0 & filterData0.word1)) )
			return PxFilterFlag::eSUPPRESS;

		pairFlags = PxPairFlag::eCONTACT_DEFAULT;
		pairFlags |= PxPairFlags(PxU16(filterData0.word2 | filterData1.word2));

		return PxFilterFlags();
	}

	void setupDrivableSurface(PxFilterData& filterData)
	{
		filterData.word3 = static_cast<PxU32>(DRIVABLE_SURFACE);
	}

	void setupNonDrivableSurface(PxFilterData& filterData)
	{
		filterData.word3 = UNDRIVABLE_SURFACE;
	}

	static PxConvexMesh* createConvexMesh(const PxVec3* verts, const PxU32 numVerts, PxPhysics& physics, PxCooking& cooking)
	{
		// Create descriptor for convex mesh
		PxConvexMeshDesc convexDesc;
		convexDesc.points.count			= numVerts;
		convexDesc.points.stride		= sizeof(PxVec3);
		convexDesc.points.data			= verts;
		convexDesc.flags				= PxConvexFlag::eCOMPUTE_CONVEX;

		PxConvexMesh* convexMesh = NULL;
		PxDefaultMemoryOutputStream buf;
		if(cooking.cookConvexMesh(convexDesc, buf))
		{
			PxDefaultMemoryInputData id(buf.getData(), buf.getSize());
			convexMesh = physics.createConvexMesh(id);
		}

		return convexMesh;
	}

	PxConvexMesh* createChassisMesh(const PxVec3 dims, PxPhysics& physics, PxCooking& cooking)
	{
		const PxF32 x = dims.x*0.5f;
		const PxF32 y = dims.y*0.5f;
		const PxF32 z = dims.z*0.5f;
		PxVec3 verts[8] =
		{
			PxVec3(x,y,-z), 
			PxVec3(x,y,z),
			PxVec3(x,-y,z),
			PxVec3(x,-y,-z),
			PxVec3(-x,y,-z), 
			PxVec3(-x,y,z),
			PxVec3(-x,-y,z),
			PxVec3(-x,-y,-z)
		};

		return createConvexMesh(verts,8,physics,cooking);
	}

	PxConvexMesh* createWheelMesh(const PxF32 width, const PxF32 radius, PxPhysics& physics, PxCooking& cooking)
	{
		PxVec3 points[2*16];
		for(PxU32 i = 0; i < 16; i++)
		{
			const PxF32 cosTheta = PxCos(i*PxPi*2.0f/16.0f);
			const PxF32 sinTheta = PxSin(i*PxPi*2.0f/16.0f);
			const PxF32 y = radius*cosTheta;
			const PxF32 z = radius*sinTheta;
			points[2*i+0] = PxVec3(-width/2.0f, y, z);
			points[2*i+1] = PxVec3(+width/2.0f, y, z);
		}

		return createConvexMesh(points,32,physics,cooking);
	}

	PxRigidDynamic* createVehicleActor
	(const PxVehicleChassisData& chassisData,
		PxMaterial** wheelMaterials, PxConvexMesh** wheelConvexMeshes, const PxU32 numWheels, const PxFilterData& wheelSimFilterData,
		PxMaterial** chassisMaterials, PxConvexMesh** chassisConvexMeshes, const PxU32 numChassisMeshes, const PxFilterData& chassisSimFilterData,
		PxPhysics& physics)
	{
		//We need a rigid body actor for the vehicle.
		//Don't forget to add the actor to the scene after setting up the associated vehicle.
		PxRigidDynamic* vehActor = physics.createRigidDynamic(PxTransform(PxIdentity));

		//Wheel and chassis query filter data.
		//Optional: cars don't drive on other cars.
		PxFilterData wheelQryFilterData;
		setupNonDrivableSurface(wheelQryFilterData);
		PxFilterData chassisQryFilterData;
		setupNonDrivableSurface(chassisQryFilterData);

		//Add all the wheel shapes to the actor.
		for(PxU32 i = 0; i < numWheels; i++)
		{
			PxConvexMeshGeometry geom(wheelConvexMeshes[i]);
			PxShape* wheelShape=PxRigidActorExt::createExclusiveShape(*vehActor, geom, *wheelMaterials[i]);
			wheelShape->setQueryFilterData(wheelQryFilterData);
			wheelShape->setSimulationFilterData(wheelSimFilterData);
			wheelShape->setLocalPose(PxTransform(PxIdentity));
		}

		//Add the chassis shapes to the actor.
		for(PxU32 i = 0; i < numChassisMeshes; i++)
		{
			PxShape* chassisShape=PxRigidActorExt::createExclusiveShape(*vehActor, PxConvexMeshGeometry(chassisConvexMeshes[i]), *chassisMaterials[i]);
			chassisShape->setQueryFilterData(chassisQryFilterData);
			chassisShape->setSimulationFilterData(chassisSimFilterData);
			chassisShape->setLocalPose(PxTransform(PxIdentity));
		}

		vehActor->setMass(chassisData.mMass);
		vehActor->setMassSpaceInertiaTensor(chassisData.mMOI);
		vehActor->setCMassLocalPose(PxTransform(chassisData.mCMOffset,PxQuat(PxIdentity)));

		return vehActor;
	}

	void computeWheelCenterActorOffsets4W(const PxF32 wheelFrontZ, const PxF32 wheelRearZ, const PxVec3& chassisDims, const PxF32 wheelWidth, const PxF32 wheelRadius, const PxU32 numWheels, PxVec3* wheelCentreOffsets)
	{
		//chassisDims.z is the distance from the rear of the chassis to the front of the chassis.
		//The front has z = -0.5*chassisDims.z and the rear has z = -0.5*chassisDims.z.
		//Compute a position for the front wheel and the rear wheel along the z-axis.
		//Compute the separation between each wheel along the z-axis.
		const PxF32 numLeftWheels = numWheels/2.0f;
		const PxF32 deltaZ = (wheelFrontZ - wheelRearZ)/(numLeftWheels-1.0f);
		//Set the outside of the left and right wheels to be flush with the chassis.
		//Set the top of the wheel to be just touching the underside of the chassis.
		//Begin by setting the rear-left/rear-right/front-left,front-right wheels.
		wheelCentreOffsets[PxVehicleDrive4WWheelOrder::eFRONT_LEFT] = PxVec3((-chassisDims.x + wheelWidth)*0.5f, -(chassisDims.y/2 + wheelRadius), wheelRearZ + 0*deltaZ*0.5f);
		wheelCentreOffsets[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT] = PxVec3((+chassisDims.x - wheelWidth)*0.5f, -(chassisDims.y/2 + wheelRadius), wheelRearZ + 0*deltaZ*0.5f);
		wheelCentreOffsets[PxVehicleDrive4WWheelOrder::eREAR_LEFT] = PxVec3((-chassisDims.x + wheelWidth)*0.5f, -(chassisDims.y/2 + wheelRadius), wheelRearZ + (numLeftWheels-1)*deltaZ);
		wheelCentreOffsets[PxVehicleDrive4WWheelOrder::eREAR_RIGHT] = PxVec3((+chassisDims.x - wheelWidth)*0.5f, -(chassisDims.y/2 + wheelRadius), wheelRearZ + (numLeftWheels-1)*deltaZ);
		//Set the remaining wheels.
		for(PxU32 i = 2, wheelCount = 4; i < numWheels-2; i+=2, wheelCount+=2)
		{
			wheelCentreOffsets[wheelCount + 0] = PxVec3((-chassisDims.x + wheelWidth)*0.5f, -(chassisDims.y/2 + wheelRadius), wheelRearZ + i*deltaZ*0.5f);
			wheelCentreOffsets[wheelCount + 1] = PxVec3((+chassisDims.x - wheelWidth)*0.5f, -(chassisDims.y/2 + wheelRadius), wheelRearZ + i*deltaZ*0.5f);
		}
	}

	void setupWheelsSimulationData
	(const PxF32 wheelMass, const PxF32 wheelMOI, const PxF32 wheelRadius, const PxF32 wheelWidth, 
		const PxU32 numWheels, const PxVec3* wheelCenterActorOffsets,
		const PxVec3& chassisCMOffset, const PxF32 chassisMass,
		PxVehicleWheelsSimData* wheelsSimData)
	{
		//Set up the wheels.
		PxVehicleWheelData wheels[PX_MAX_NB_WHEELS];
		{
			//Set up the wheel data structures with mass, moi, radius, width.
			for(PxU32 i = 0; i < numWheels; i++)
			{
				wheels[i].mMass = wheelMass;
				wheels[i].mMOI = wheelMOI;
				wheels[i].mRadius = wheelRadius;
				wheels[i].mWidth = wheelWidth;
			}

			//Enable the handbrake for the rear wheels only.
			wheels[PxVehicleDrive4WWheelOrder::eREAR_LEFT].mMaxHandBrakeTorque=4000.0f;
			wheels[PxVehicleDrive4WWheelOrder::eREAR_RIGHT].mMaxHandBrakeTorque=4000.0f;
			//Enable steering for the front wheels only.
			wheels[PxVehicleDrive4WWheelOrder::eFRONT_LEFT].mMaxSteer=PxPi*0.3333f;
			wheels[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT].mMaxSteer=PxPi*0.3333f;
		}

		//Set up the tires.
		PxVehicleTireData tires[PX_MAX_NB_WHEELS];
		{
			//Set up the tires.
			for(PxU32 i = 0; i < numWheels; i++)
			{
				tires[i].mType = 0;
			}
		}

		//Set up the suspensions
		PxVehicleSuspensionData suspensions[PX_MAX_NB_WHEELS];
		{
			//Compute the mass supported by each suspension spring.
			PxF32 suspSprungMasses[PX_MAX_NB_WHEELS];
			PxVehicleComputeSprungMasses
			(numWheels, wheelCenterActorOffsets, 
				chassisCMOffset, chassisMass, 1, suspSprungMasses);

			//Set the suspension data.
			for(PxU32 i = 0; i < numWheels; i++)
			{
				suspensions[i].mMaxCompression = 0.3f;
				suspensions[i].mMaxDroop = 0.1f;
				suspensions[i].mSpringStrength = 35000.0f;	
				suspensions[i].mSpringDamperRate = 4500.0f;
				suspensions[i].mSprungMass = suspSprungMasses[i];
			}

			//Set the camber angles.
			const PxF32 camberAngleAtRest=0.0;
			const PxF32 camberAngleAtMaxDroop=0.01f;
			const PxF32 camberAngleAtMaxCompression=-0.01f;
			for(PxU32 i = 0; i < numWheels; i+=2)
			{
				suspensions[i + 0].mCamberAtRest =  camberAngleAtRest;
				suspensions[i + 1].mCamberAtRest =  -camberAngleAtRest;
				suspensions[i + 0].mCamberAtMaxDroop = camberAngleAtMaxDroop;
				suspensions[i + 1].mCamberAtMaxDroop = -camberAngleAtMaxDroop;
				suspensions[i + 0].mCamberAtMaxCompression = camberAngleAtMaxCompression;
				suspensions[i + 1].mCamberAtMaxCompression = -camberAngleAtMaxCompression;
			}
		}

		//Set up the wheel geometry.
		PxVec3 suspTravelDirections[PX_MAX_NB_WHEELS];
		PxVec3 wheelCentreCMOffsets[PX_MAX_NB_WHEELS];
		PxVec3 suspForceAppCMOffsets[PX_MAX_NB_WHEELS];
		PxVec3 tireForceAppCMOffsets[PX_MAX_NB_WHEELS];
		{
			//Set the geometry data.
			for(PxU32 i = 0; i < numWheels; i++)
			{
				//Vertical suspension travel.
				suspTravelDirections[i] = PxVec3(0,-1,0);

				//Wheel center offset is offset from rigid body center of mass.
				wheelCentreCMOffsets[i] = 
					wheelCenterActorOffsets[i] - chassisCMOffset;

				//Suspension force application point 0.3 metres below 
				//rigid body center of mass.
				suspForceAppCMOffsets[i] =
					PxVec3(wheelCentreCMOffsets[i].x,-0.3f,wheelCentreCMOffsets[i].z);

				//Tire force application point 0.3 metres below 
				//rigid body center of mass.
				tireForceAppCMOffsets[i] =
					PxVec3(wheelCentreCMOffsets[i].x,-0.3f,wheelCentreCMOffsets[i].z);
			}
		}

		//Set up the filter data of the raycast that will be issued by each suspension.
		PxFilterData qryFilterData;
		setupNonDrivableSurface(qryFilterData);

		//Set the wheel, tire and suspension data.
		//Set the geometry data.
		//Set the query filter data
		for(PxU32 i = 0; i < numWheels; i++)
		{
			wheelsSimData->setWheelData(i, wheels[i]);
			wheelsSimData->setTireData(i, tires[i]);
			wheelsSimData->setSuspensionData(i, suspensions[i]);
			wheelsSimData->setSuspTravelDirection(i, suspTravelDirections[i]);
			wheelsSimData->setWheelCentreOffset(i, wheelCentreCMOffsets[i]);
			wheelsSimData->setSuspForceAppPointOffset(i, suspForceAppCMOffsets[i]);
			wheelsSimData->setTireForceAppPointOffset(i, tireForceAppCMOffsets[i]);
			wheelsSimData->setSceneQueryFilterData(i, qryFilterData);
			wheelsSimData->setWheelShapeMapping(i, PxI32(i)); 
		}

		//Add a front and rear anti-roll bar
		PxVehicleAntiRollBarData barFront;
		barFront.mWheel0 = PxVehicleDrive4WWheelOrder::eFRONT_LEFT;
		barFront.mWheel1 = PxVehicleDrive4WWheelOrder::eFRONT_RIGHT;
		barFront.mStiffness = 10000.0f;
		wheelsSimData->addAntiRollBarData(barFront);
		PxVehicleAntiRollBarData barRear;
		barRear.mWheel0 = PxVehicleDrive4WWheelOrder::eREAR_LEFT;
		barRear.mWheel1 = PxVehicleDrive4WWheelOrder::eREAR_RIGHT;
		barRear.mStiffness = 10000.0f;
		wheelsSimData->addAntiRollBarData(barRear);
	}

	PxVehicleDrive4W* createVehicle4W(const VehicleDesc& vehicle4WDesc, PxPhysics* physics, PxCooking* cooking)
	{
		const PxVec3 chassisDims = vehicle4WDesc.chassisDims;
		const PxF32 wheelWidth = vehicle4WDesc.wheelWidth;
		const PxF32 wheelRadius = vehicle4WDesc.wheelRadius;
		const PxU32 numWheels = vehicle4WDesc.numWheels;

		const PxFilterData& chassisSimFilterData = vehicle4WDesc.chassisSimFilterData;
		const PxFilterData& wheelSimFilterData = vehicle4WDesc.wheelSimFilterData;

		//Construct a physx actor with shapes for the chassis and wheels.
		//Set the rigid body mass, moment of inertia, and center of mass offset.
		PxRigidDynamic* veh4WActor = NULL;
		{
			//Construct a convex mesh for a cylindrical wheel.
			PxConvexMesh* wheelMesh = createWheelMesh(wheelWidth, wheelRadius, *physics, *cooking);
			//Assume all wheels are identical for simplicity.
			PxConvexMesh* wheelConvexMeshes[PX_MAX_NB_WHEELS];
			PxMaterial* wheelMaterials[PX_MAX_NB_WHEELS];

			//Set the meshes and materials for the driven wheels.
			for(PxU32 i = PxVehicleDrive4WWheelOrder::eFRONT_LEFT; i <= PxVehicleDrive4WWheelOrder::eREAR_RIGHT; i++)
			{
				wheelConvexMeshes[i] = wheelMesh;
				wheelMaterials[i] = vehicle4WDesc.wheelMaterial;
			}
			//Set the meshes and materials for the non-driven wheels
			for(PxU32 i = PxVehicleDrive4WWheelOrder::eREAR_RIGHT + 1; i < numWheels; i++)
			{
				wheelConvexMeshes[i] = wheelMesh;
				wheelMaterials[i] = vehicle4WDesc.wheelMaterial;
			}

			//Chassis just has a single convex shape for simplicity.
			PxConvexMesh* chassisConvexMesh = createChassisMesh(chassisDims, *physics, *cooking);
			PxConvexMesh* chassisConvexMeshes[1] = {chassisConvexMesh};
			PxMaterial* chassisMaterials[1] = {vehicle4WDesc.chassisMaterial};

			//Rigid body data.
			PxVehicleChassisData rigidBodyData;
			rigidBodyData.mMOI = vehicle4WDesc.chassisMOI;
			rigidBodyData.mMass = vehicle4WDesc.chassisMass;
			rigidBodyData.mCMOffset = vehicle4WDesc.chassisCMOffset;

			veh4WActor = createVehicleActor
			(rigidBodyData,
				wheelMaterials, wheelConvexMeshes, numWheels, wheelSimFilterData,
				chassisMaterials, chassisConvexMeshes, 1, chassisSimFilterData,
				*physics);
		}

		//Set up the sim data for the wheels.
		PxVehicleWheelsSimData* wheelsSimData = PxVehicleWheelsSimData::allocate(numWheels);
		{
			//Compute the wheel center offsets from the origin.
			PxVec3 wheelCenterActorOffsets[PX_MAX_NB_WHEELS];
			const PxF32 frontZ = chassisDims.z*0.3f;
			const PxF32 rearZ = -chassisDims.z*0.3f;
			computeWheelCenterActorOffsets4W(frontZ, rearZ, chassisDims, wheelWidth, wheelRadius, numWheels, wheelCenterActorOffsets);

			//Set up the simulation data for all wheels.
			setupWheelsSimulationData
			(vehicle4WDesc.wheelMass, vehicle4WDesc.wheelMOI, wheelRadius, wheelWidth, 
				numWheels, wheelCenterActorOffsets,
				vehicle4WDesc.chassisCMOffset, vehicle4WDesc.chassisMass,
				wheelsSimData);
		}

		//Set up the sim data for the vehicle drive model.
		PxVehicleDriveSimData4W driveSimData;
		{
			//Diff
			PxVehicleDifferential4WData diff;
			diff.mType=PxVehicleDifferential4WData::eDIFF_TYPE_LS_4WD;
			driveSimData.setDiffData(diff);

			//Engine
			PxVehicleEngineData engine;
			engine.mPeakTorque=500.0f;
			engine.mMaxOmega=600.0f;//approx 6000 rpm
			driveSimData.setEngineData(engine);

			//Gears
			PxVehicleGearsData gears;
			gears.mSwitchTime=0.1f;
			driveSimData.setGearsData(gears);

			//Clutch
			PxVehicleClutchData clutch;
			clutch.mStrength=10.0f;
			driveSimData.setClutchData(clutch);

			//Ackermann steer accuracy
			PxVehicleAckermannGeometryData ackermann;
			ackermann.mAccuracy=1.0f;
			ackermann.mAxleSeparation=
				wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_LEFT).z-
				wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_LEFT).z;
			ackermann.mFrontWidth=
				wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_RIGHT).x-
				wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_LEFT).x;
			ackermann.mRearWidth=
				wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_RIGHT).x -
				wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_LEFT).x;
			driveSimData.setAckermannGeometryData(ackermann);
		}

		//Create a vehicle from the wheels and drive sim data.
		PxVehicleDrive4W* vehDrive4W = PxVehicleDrive4W::allocate(numWheels);
		vehDrive4W->setup(physics, veh4WActor, *wheelsSimData, driveSimData, numWheels - 4);

		assert(veh4WActor == vehDrive4W->getRigidDynamicActor());

		//Configure the userdata
		//configureUserData(vehDrive4W, vehicle4WDesc.actorUserData, vehicle4WDesc.shapeUserDatas);

		//Free the sim data because we don't need that any more.
		wheelsSimData->free();

		return vehDrive4W;
	}

	VehicleDesc InitVehicleDesc(PxMaterial* material)
	{
		//Set up the chassis mass, dimensions, moment of inertia, and center of mass offset.
		//The moment of inertia is just the moment of inertia of a cuboid but modified for easier steering.
		//Center of mass offset is 0.65m above the base of the chassis and 0.25m towards the front.
		const PxF32 chassisMass = 1500.0f; 
		const PxVec3 chassisDims(2.5f,2.0f,5.0f);
		const PxVec3 chassisMOI
		((chassisDims.y*chassisDims.y + chassisDims.z*chassisDims.z)*chassisMass/12.0f,
			(chassisDims.x*chassisDims.x + chassisDims.z*chassisDims.z)*0.8f*chassisMass/12.0f,
			(chassisDims.x*chassisDims.x + chassisDims.y*chassisDims.y)*chassisMass/12.0f);
		const PxVec3 chassisCMOffset(0.0f, -chassisDims.y*0.5f + 0.65f, 0.25f);

		//Set up the wheel mass, radius, width, moment of inertia, and number of wheels.
		//Moment of inertia is just the moment of inertia of a cylinder.
		const PxF32 wheelMass = 20.0f;
		const PxF32 wheelRadius = 0.5f;
		const PxF32 wheelWidth = 0.4f;
		const PxF32 wheelMOI = 0.5f*wheelMass*wheelRadius*wheelRadius;
		const PxU32 nbWheels = 4;

		VehicleDesc vehicleDesc;

		vehicleDesc.chassisMass = chassisMass;
		vehicleDesc.chassisDims = chassisDims;
		vehicleDesc.chassisMOI = chassisMOI;
		vehicleDesc.chassisCMOffset = chassisCMOffset;
		vehicleDesc.chassisMaterial = material;
		vehicleDesc.chassisSimFilterData = PxFilterData(COLLISION_FLAG_CHASSIS, COLLISION_FLAG_CHASSIS_AGAINST, 0, 0);

		vehicleDesc.wheelMass = wheelMass;
		vehicleDesc.wheelRadius = wheelRadius;
		vehicleDesc.wheelWidth = wheelWidth;
		vehicleDesc.wheelMOI = wheelMOI;
		vehicleDesc.numWheels = nbWheels;
		vehicleDesc.wheelMaterial = material;
		vehicleDesc.chassisSimFilterData = PxFilterData(COLLISION_FLAG_WHEEL, COLLISION_FLAG_WHEEL_AGAINST, 0, 0);

		return vehicleDesc;
	}

	ECS::EntityID CreateDrivablePlane(ECS::EntityManager& manager, Physics::PhysXManager& physics, const PxFilterData& simFilterData, PxMaterial* material)
	{
		auto plane = physics.CreatePlaneStatic(vec3(), glm::rotate(quat(), (float)M_PI / 2, vec3(0, 0, 1)));

		//Get the plane shape so we can set query and simulation filter data.
		PxShape* shapes[1];
		plane->getShapes(shapes, 1);

		//Set the query filter data of the ground plane so that the vehicle raycasts can hit the ground.
		PxFilterData qryFilterData;
		setupDrivableSurface(qryFilterData);
		shapes[0]->setQueryFilterData(qryFilterData);

		//Set the simulation filter data of the ground plane so that it collides with the chassis of a vehicle but not the wheels.
		shapes[0]->setSimulationFilterData(simFilterData);

		auto entity = manager.CreateEntity();
		auto rigidbody = manager.AddComponent<ECS::components::RigidbodyStatic>(entity);
		rigidbody->body = std::move(plane);

		auto transform = manager.AddComponent<ECS::components::Transform>(entity);

		return plane;
	}

	ECS::EntityID CreateVehicle(ECS::EntityManager& manager, PhysXManager& physics, vec3 position, quat rotation, VehicleDesc& desk)
	{
		PxTransform startTransform(PxVec3(0, (desk.chassisDims.y*0.5f + desk.wheelRadius + 1.0f), 0), PxQuat(PxIdentity));
		startTransform.p += Convert(position);
		startTransform.q *= Convert(rotation);

		auto entity = manager.CreateEntity();
		auto vehicle = manager.AddComponent<ECS::components::Vehicle>(entity);
		
		auto vehicle_physics = createVehicle4W(desk, physics.GetPhysX(), physics.GetCooking());
		vehicle->vehicle = vehicle_physics;

		auto rigidbody = manager.AddComponent<ECS::components::RigidbodyDynamic>(entity);

		rigidbody->body = Handle(vehicle_physics->getRigidDynamicActor());
		rigidbody->body->setGlobalPose(startTransform);
		physics.GetScene()->addActor(*rigidbody->body);

		//Set the vehicle to rest in first gear.
		//Set the vehicle to use auto-gears.
		vehicle_physics->setToRestState();
		vehicle_physics->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
		vehicle_physics->mDriveDynData.setUseAutoGears(true);

		auto transform = manager.AddComponent<ECS::components::Transform>(entity);
		ConvertTransform(startTransform, transform->position, transform->rotation);
		transform->bounds = AABB(-Convert(desk.chassisDims * 0.5f) + vec3(0, -desk.wheelRadius * 2, 0), Convert(desk.chassisDims * 0.5f));

		return entity;
	}

	//Tire model friction for each combination of drivable surface type and tire type.
	static PxF32 TireFrictionMultipliers[MAX_NUM_SURFACE_TYPES][MAX_NUM_TIRE_TYPES] =
	{
		//NORMAL,	WORN
		{2.00f,		0.1f}//TARMAC
	};

	Handle<PxVehicleDrivableSurfaceToTireFrictionPairs> CreateFrictionPairs(const PxMaterial& defaultMaterial)
	{
		PxVehicleDrivableSurfaceType surfaceTypes[1];
		surfaceTypes[0].mType = SURFACE_TYPE_TARMAC;

		const PxMaterial* surfaceMaterials[1];
		surfaceMaterials[0] = &defaultMaterial;

		PxVehicleDrivableSurfaceToTireFrictionPairs* surfaceTirePairs =
			PxVehicleDrivableSurfaceToTireFrictionPairs::allocate(MAX_NUM_TIRE_TYPES, MAX_NUM_SURFACE_TYPES);

		surfaceTirePairs->setup(MAX_NUM_TIRE_TYPES, MAX_NUM_SURFACE_TYPES, surfaceMaterials, surfaceTypes);

		for (PxU32 i = 0; i < MAX_NUM_SURFACE_TYPES; i++)
		{
			for (PxU32 j = 0; j < MAX_NUM_TIRE_TYPES; j++)
			{
				surfaceTirePairs->setTypePairFriction(i, j, TireFrictionMultipliers[i][j]);
			}
		}
		return surfaceTirePairs;
	}

	VehicleDataCache::VehicleDataCache(PxScene& scene, PxAllocatorCallback& allocator, PxMaterial& material)
		: allocator(allocator)
	{
		scene_query = VehicleSceneQueryData::allocate(1, PX_MAX_NB_WHEELS, 1, 1, WheelSceneQueryPreFilterBlocking, NULL, allocator);
		batch_query = VehicleSceneQueryData::setUpBatchedSceneQuery(0, *scene_query, &scene);
		friction_pairs = CreateFrictionPairs(material);
	}

	VehicleDataCache::~VehicleDataCache()
	{
		scene_query->free(allocator);
	}

}