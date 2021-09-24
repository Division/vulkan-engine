#include "Player.h"
#include "Controller.h"
#include "resources/SkeletalAnimationResource.h"
#include <ecs/components/AnimationController.h>
#include <ecs/components/Transform.h>
#include "projectile/Projectile.h"
#include "TopDownShooter.h"

#include "Engine.h"
#include "system/Input.h"

using namespace Resources;
using namespace ECS;

namespace scene
{

	void PlayerBehaviour::Awake()
	{
		auto controller = GetBehaviourStrong<CharacterController>();

		typedef CharacterController::MoveAnimationType MoveAnimationType;
		typedef CharacterController::StationaryAnimationType StationaryAnimationType;

		CharacterController::Animations animations;
		animations.stationary[StationaryAnimationType::Idle] = SkeletalAnimationResource::Handle(L"assets/top-down-shooter/characters/uetest/Rifle@Idle.anim");
		animations.stationary[StationaryAnimationType::IdleAim] = SkeletalAnimationResource::Handle(L"assets/top-down-shooter/characters/uetest/Rifle@Idle_GunDown.anim");
		animations.stationary[StationaryAnimationType::Shoot] = SkeletalAnimationResource::Handle(L"assets/top-down-shooter/characters/uetest/Rifle@ShootLoop_Additive.anim");

		animations.move[MoveAnimationType::RunForward] = SkeletalAnimationResource::Handle(L"assets/top-down-shooter/characters/uetest/Rifle@RunFwdLoop.anim");
		animations.move[MoveAnimationType::WalkForward] = SkeletalAnimationResource::Handle(L"assets/top-down-shooter/characters/uetest/Rifle@WalkFwdLoop.anim");
		animations.move[MoveAnimationType::RunBackward] = SkeletalAnimationResource::Handle(L"assets/top-down-shooter/characters/uetest/Rifle@RunBwdLoop.anim");
		animations.move[MoveAnimationType::RunLeft] = SkeletalAnimationResource::Handle(L"assets/top-down-shooter/characters/uetest/Rifle@StrafeRunLeftLoop.anim");
		animations.move[MoveAnimationType::RunRight] = SkeletalAnimationResource::Handle(L"assets/top-down-shooter/characters/uetest/Rifle@StrafeRunRightLoop.anim");
		animations.move[MoveAnimationType::RunBackwardRight] = SkeletalAnimationResource::Handle(L"assets/top-down-shooter/characters/uetest/Rifle@StrafeRun135RightLoop.anim");
		animations.move[MoveAnimationType::RunForwardRight] = SkeletalAnimationResource::Handle(L"assets/top-down-shooter/characters/uetest/Rifle@StrafeRun45RightLoop.anim");
		animations.move[MoveAnimationType::RunBackwardLeft] = SkeletalAnimationResource::Handle(L"assets/top-down-shooter/characters/uetest/Rifle@StrafeRun135LeftLoop.anim");
		animations.move[MoveAnimationType::RunForwardLeft] = SkeletalAnimationResource::Handle(L"assets/top-down-shooter/characters/uetest/Rifle@StrafeRun45LeftLoop.anim");
		controller->Initialize(animations);
	}

	void PlayerBehaviour::Shoot(components::Transform* player_transform, CharacterController* character_controller)
	{
		::Projectile::Params params;
		params.position = player_transform->position + vec3(0, 1.0f, 0);
		params.direction = vec3(character_controller->GetCurrentAimDir().x, 0, character_controller->GetCurrentAimDir().y);
		params.speed = 30.0f;
		params.max_distance = 10.0f;
		params.dimensions = vec2(0.4f, 0.1f) * 0.5f;

		Game::GetInstance()->GetProjectileManager()->CreateProjectile(params);
	}

	void PlayerBehaviour::Update(float dt)
	{
		auto input = Engine::Get()->GetInput();

		auto character_controller = GetBehaviour<CharacterController>().lock();
		auto transform = GetComponent<components::Transform>();
		CharacterController::Input character_input;
		character_input.aim_direction = vec2(0);
		character_input.move_direction = vec2(0);
		character_input.shoot = false;

		// TODO: don't depend on Game
		if (Game::GetInstance()->IsCameraControl())
			return;

		auto mouse_target = Game::GetInstance()->GetMouseTarget();

		if (mouse_target)
		{
			auto dir = *mouse_target - transform->position;
			float length = glm::length(dir);
			if (length > 0.001f)
			{
				dir /= length;
				character_input.aim_direction = vec2(dir.x, dir.z);
			}
		}

		if (input->keyDown(::System::Key::Up) || input->keyDown(::System::Key::W))
			character_input.move_direction.y = -1;
		else if (input->keyDown(::System::Key::Down) || input->keyDown(::System::Key::S))
			character_input.move_direction.y = 1;
		if (input->keyDown(::System::Key::Left) || input->keyDown(::System::Key::A))
			character_input.move_direction.x = -1;
		else if (input->keyDown(::System::Key::Right) || input->keyDown(::System::Key::D))
			character_input.move_direction.x = 1;

		float length = glm::length(character_input.move_direction);
		if (length > 0.001f)
			character_input.move_direction /= length;

		character_input.shoot = input->keyDown(::System::Key::MouseLeft);
		auto time = Engine::Get()->time();
		if (character_input.shoot && (time - last_shoot_time > 0.1))
		{
			Shoot(transform, character_controller.get());
			last_shoot_time = time;
		}

		character_controller->SetInput(character_input);
	}

}