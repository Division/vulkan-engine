#pragma once

#include "resources/SkeletalAnimationResource.h"
#include "render/animation/SkeletalAnimation.h"
#include <magic_enum/magic_enum.hpp>
#include "ecs/System.h"


namespace ECS::components
{
	struct AnimationController;
	struct Transform;

	struct CharacterController
	{
		static constexpr uint32_t STATIONARY_ANIM_LAYER = 4;
		static constexpr uint32_t MOVE_ANIM_LAYER = 5;
		static constexpr uint32_t ADDITIVE_LAYER = 6;

		struct Input
		{
			vec2 move_direction = vec2(0);
			vec2 aim_direction = vec2(0);
			bool shoot = false;
		};

		enum StationaryAnimationType
		{
			Idle,
			IdleAim,
			Shoot
		};

		enum MoveAnimationType
		{
			RunForward,
			RunForwardRight,
			RunRight,
			RunBackwardRight,
			RunBackward,
			RunForwardLeft,
			RunLeft,
			RunBackwardLeft,
			WalkForward
		};

		enum class State
		{
			Idle,
			IdleAim,
			Move
		};

		inline static constexpr MoveAnimationType RUN_INDICES[] = 
		{ 
			RunForward,
			RunForwardRight,
			RunRight,
			RunBackwardRight,
			RunBackward,
			RunForward,
			RunForwardLeft,
			RunLeft,
			RunBackwardLeft,
			RunBackward
		};

		Input input;
		std::array<Resources::SkeletalAnimationResource::Handle, magic_enum::enum_count<MoveAnimationType>()> move_animations;
		std::array<Resources::SkeletalAnimationResource::Handle, magic_enum::enum_count<StationaryAnimationType>()> stationary_animations;
		std::array<SkeletalAnimation::AnimationInstance::Handle, magic_enum::enum_count<MoveAnimationType>()> move_playback_handles;
		std::array<float, magic_enum::enum_count<MoveAnimationType>()> move_target_weights;
		SkeletalAnimation::AnimationInstance::Handle shoot_animation_handle;

		State state = State::Idle;
		vec2 current_aim_dir = vec2(0, 1);
		void ClearMoveTargets();
		void ResetMovePlayback();
		void Startup(AnimationController* animation_controller);
	};

	class CharacterControllerTemplate : public ComponentTemplate
	{
		virtual void Load(const rapidjson::Value& data) override
		{
		}

		virtual void Spawn(EntityManager& manager, EntityID entity, vec3 position) override
		{
			manager.AddComponent<CharacterController>(entity);
		}
	};
}

namespace ECS::systems
{
	class CharacterControllerSystem : public ECS::System
	{
	public:
		CharacterControllerSystem(ECS::EntityManager& manager)
			: ECS::System(manager, false)
		{}

		void Process(ECS::Chunk* chunk) override;

	private:
		void ProcessController(components::CharacterController* character_controller, components::AnimationController* animation_controller, components::Transform* transform);
		void ProcessShooting(components::CharacterController* character_controller, components::AnimationController* animation_controller, components::Transform* transform);
		void ProcessIdle(components::CharacterController* character_controller, components::AnimationController* animation_controller, components::Transform* transform);
		void ProcessMovement(components::CharacterController* character_controller, components::AnimationController* animation_controller, components::Transform* transform);
		void UpdateMovementWeights(components::CharacterController* character_controller);
		void UpdateDirection(components::CharacterController* character_controller, components::Transform* transform);
	};

}