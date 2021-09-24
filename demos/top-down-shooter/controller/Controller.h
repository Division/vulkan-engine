#pragma once

#include "resources/SkeletalAnimationResource.h"
#include "render/animation/SkeletalAnimation.h"
#include <magic_enum/magic_enum.hpp>
#include "ecs/System.h"
#include "scene/Behaviour.h"

namespace ECS::components
{
	struct AnimationController;
	struct Transform;
}

namespace scene
{
	class CharacterController : public Behaviour
	{
	public:
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

		struct Animations
		{
			std::array<Resources::SkeletalAnimationResource::Handle, magic_enum::enum_count<MoveAnimationType>()> move;
			std::array<Resources::SkeletalAnimationResource::Handle, magic_enum::enum_count<StationaryAnimationType>()> stationary;
		};

		void ClearMoveTargets();
		void ResetMovePlayback();
		
		void Initialize(Animations animations);
		void Update(float dt) override;
		//int ExecutionOrder() const override { return 1001; }

		void SetInput(const Input& value) { input = value; }

		vec2 GetCurrentAimDir() const { return current_aim_dir; }

	private:
		void ProcessController(ECS::components::AnimationController* animation_controller, ECS::components::Transform* transform);
		void ProcessShooting(ECS::components::AnimationController* animation_controller, ECS::components::Transform* transform);
		void ProcessIdle(ECS::components::AnimationController* animation_controller, ECS::components::Transform* transform);
		void ProcessMovement(ECS::components::AnimationController* animation_controller, ECS::components::Transform* transform);
		void UpdateMovementWeights();
		void UpdateDirection(ECS::components::Transform* transform);

		Input input;
		Animations animations;
		std::array<SkeletalAnimation::AnimationInstance::Handle, magic_enum::enum_count<MoveAnimationType>()> move_playback_handles;
		std::array<float, magic_enum::enum_count<MoveAnimationType>()> move_target_weights;
		SkeletalAnimation::AnimationInstance::Handle shoot_animation_handle;

		State state = State::Idle;
		vec2 current_aim_dir = vec2(0, 1);
		bool is_initialized = false;
	};

}
