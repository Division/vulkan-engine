#include "ControlsSystem.h"
#include "Engine.h"
#include "system/Input.h"
#include "ecs/components/Physics.h"
#include "ecs/components/Static.h"
#include "components/PlayerInput.h"
#include "entities/vehicle/VehicleUtils.h"

using namespace physx;
using namespace Physics;
using namespace ECS;
using namespace Vehicle::Utils;

namespace ECS::systems
{

	void ControlsSystem::Process(ECS::Chunk* chunk)
	{
		auto input = Engine::Get()->GetInput();

		ComponentFetcher<components::PlayerInput> input_fetcher(*chunk);
		ComponentFetcher<components::Vehicle> vehicle_fetcher(*chunk);

		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{
			auto player_input = input_fetcher.GetComponent(i);
			auto vehicle = vehicle_fetcher.GetComponent(i);
			player_input->accelerate = input->keyDown(::System::Key::Up);
			player_input->brake = input->keyDown(::System::Key::Down);
			player_input->turn_left = input->keyDown(::System::Key::Left);
			player_input->turn_right = input->keyDown(::System::Key::Right);

			vehicle->input.setDigitalAccel(player_input->accelerate);
			vehicle->input.setDigitalBrake(player_input->brake);
			vehicle->input.setDigitalSteerLeft(player_input->turn_right);
			vehicle->input.setDigitalSteerRight(player_input->turn_left);
		}
	}

	namespace
	{
		PxVehicleKeySmoothingData KeySmoothingData =
		{
			{
				6.0f,	//rise rate eANALOG_INPUT_ACCEL
				6.0f,	//rise rate eANALOG_INPUT_BRAKE		
				6.0f,	//rise rate eANALOG_INPUT_HANDBRAKE	
				2.5f,	//rise rate eANALOG_INPUT_STEER_LEFT
				2.5f,	//rise rate eANALOG_INPUT_STEER_RIGHT
			},
			{
				10.0f,	//fall rate eANALOG_INPUT_ACCEL
				10.0f,	//fall rate eANALOG_INPUT_BRAKE		
				10.0f,	//fall rate eANALOG_INPUT_HANDBRAKE	
				5.0f,	//fall rate eANALOG_INPUT_STEER_LEFT
				5.0f	//fall rate eANALOG_INPUT_STEER_RIGHT
			}
		};

		PxF32 SteerVsForwardSpeedData[2*8]=
		{
			0.0f,		0.75f,
			5.0f,		0.75f,
			30.0f,		0.125f,
			120.0f,		0.1f,
			PX_MAX_F32, PX_MAX_F32,
			PX_MAX_F32, PX_MAX_F32,
			PX_MAX_F32, PX_MAX_F32,
			PX_MAX_F32, PX_MAX_F32
		};
		PxFixedSizeLookupTable<8> SteerVsForwardSpeedTable(SteerVsForwardSpeedData,4);
	}

	void VehicleControlSystem::Process(Chunk* chunk)
	{
		OPTICK_EVENT();
		ComponentFetcher<components::Vehicle> vehicle_fetcher(*chunk);

		auto* data_cache = manager.GetStaticComponent<VehicleDataCache>();
		auto* scene_query = data_cache->GetSceneQueryData();
		auto* batch_query = data_cache->GetBatchQuery();
		auto* friction_pairs = data_cache->GetFrictionPairs();

		const float dt = manager.GetStaticComponent<components::DeltaTime>()->physics_dt;

		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{
			auto* vehicle = vehicle_fetcher.GetComponent(i);

			physx::PxVehicleDrive4WSmoothDigitalRawInputsAndSetAnalogInputs(KeySmoothingData, SteerVsForwardSpeedTable, vehicle->input, dt, false, *vehicle->vehicle);

			//Raycasts.
			PxVehicleWheels* vehicles[1] = { vehicle->vehicle };
			PxRaycastQueryResult* raycastResults = scene_query->getRaycastQueryResultBuffer(0);
			const PxU32 raycastResultsSize = scene_query->getQueryResultBufferSize();
			PxVehicleSuspensionRaycasts(batch_query, 1, vehicles, raycastResultsSize, raycastResults);

			//Vehicle update.
			const PxVec3 grav(0, -10, 0);
			PxWheelQueryResult wheelQueryResults[PX_MAX_NB_WHEELS];
			PxVehicleWheelQueryResult vehicleQueryResults[1] = {{wheelQueryResults, vehicle->vehicle->mWheelsSimData.getNbWheels()}};
			PxVehicleUpdates(dt, grav, *friction_pairs, 1, vehicles, vehicleQueryResults);

			/*


				//Work out if the vehicle is in the air.
				gIsVehicleInAir = gVehicle4W->getRigidDynamicActor()->isSleeping() ? false : PxVehicleIsInAir(vehicleQueryResults[0]);
			*/
		}
	}
}