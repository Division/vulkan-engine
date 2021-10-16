#include "Nightmare.h"
#include "Monster.h"
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

	void NightmareBehaviour::Awake()
	{
		monster = GetBehaviourStrong<MonsterController>().get();
		MonsterController::Animations animations;
		animations[MonsterController::AnimationType::Idle] = SkeletalAnimationResource::Handle(L"assets/top-down-shooter/characters/monsters/nightmare/nightmare_creature@battle_idle.anim");
		animations[MonsterController::AnimationType::Attack] = SkeletalAnimationResource::Handle(L"assets/top-down-shooter/characters/monsters/nightmare/nightmare_creature@attack_1.anim");
		animations[MonsterController::AnimationType::Move] = SkeletalAnimationResource::Handle(L"assets/top-down-shooter/characters/monsters/nightmare/nightmare_creature@Run_root_motion.anim");
		animations[MonsterController::AnimationType::Transition] = SkeletalAnimationResource::Handle(L"assets/top-down-shooter/characters/monsters/nightmare/nightmare_creature@crawl_to_stand.anim");
		monster->Initialize(animations);
	}

	void NightmareBehaviour::ProcessIdle(float distance_to_player)
	{
		monster->SetState(MonsterController::State::Idle);
	}

	void NightmareBehaviour::ProcessChase(float distance_to_player, vec3 player_position, vec3 position)
	{
		monster->SetState(MonsterController::State::Move);
	}

	void NightmareBehaviour::Update(float dt)
	{
		return;
		const auto player_position = Game::GetInstance()->GetPlayerPosition();
		const auto position = GetComponent<components::Transform>()->position;
		const auto distance_to_player = glm::distance(player_position, position);

		switch (state)
		{
		case State::Idle:
			ProcessIdle(distance_to_player);
			break;

		case State::Chase:
			ProcessChase(distance_to_player, player_position, position);
			break;
		}
	}
}