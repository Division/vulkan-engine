#include "Controller.h"
#include "ecs/components/AnimationController.h"
#include "ecs/components/Transform.h"
#include "ecs/components/Static.h"
#include <glm/gtx/rotate_vector.hpp>

#include "Engine.h"
#include "render/debug/DebugDraw.h"

namespace scene
{
	using namespace ECS;

	void CharacterController::Initialize(Animations _animations)
	{
		animations = _animations;

		auto animation_controller = GetComponent<components::AnimationController>();
		animation_controller->mixer->SetRootMotionEnabled(true);

		const auto move_params = SkeletalAnimation::PlaybackParams().Playback(SkeletalAnimation::PlaybackMode::Loop).FadeTime(0.0f).Layer(MOVE_ANIM_LAYER);

		// move animations are always active and manipulated by handles. Zero weight animation has zero cost.
		for (uint32_t i = 0; i < animations.move.size(); i++)
		{
			move_playback_handles[i] = animation_controller->mixer->BlendAnimation(animations.move[i], move_params);
			move_playback_handles[i].SetWeight(0);
		}

		move_target_weights.fill(0);

		const auto stationary_params = SkeletalAnimation::PlaybackParams().Loop().FadeTime(0.0f).Layer(STATIONARY_ANIM_LAYER);
		animation_controller->mixer->PlayAnimation(animations.stationary[StationaryAnimationType::Idle], stationary_params);

		is_initialized = true;
	}

	void CharacterController::ClearMoveTargets()
	{
		for (uint32_t i = 0; i < move_target_weights.size(); i++)
			move_target_weights[i] = 0;
	}

	void CharacterController::ResetMovePlayback()
	{
		for (auto& handle : move_playback_handles)
			handle.SetProgress(0);
	}

	void CharacterController::Update(float dt)
	{
		if (!is_initialized)
			return;

		auto* animation_controller = GetComponent<components::AnimationController>();
		auto* transform = GetComponent<components::Transform>();
		ProcessController(animation_controller, transform);
	}

	void CharacterController::ProcessShooting(components::AnimationController* animation_controller, components::Transform* transform)
	{
		if (input.shoot && !shoot_animation_handle)
		{
			const auto params = SkeletalAnimation::PlaybackParams()
									.Playback(SkeletalAnimation::PlaybackMode::Loop)
									.Blending(SkeletalAnimation::BlendingMode::Additive)
									.FadeTime(0.15f).Layer(ADDITIVE_LAYER);
			auto& animation = animations.stationary[StationaryAnimationType::Shoot];
			shoot_animation_handle = animation_controller->mixer->PlayAnimation(animation, params);
		}
		else if (!input.shoot && shoot_animation_handle)
		{
			shoot_animation_handle.FadeOut(0.15);
			shoot_animation_handle = nullptr;
		}
	}

	void CharacterController::ProcessController(components::AnimationController* animation_controller, components::Transform* transform)
	{
		ProcessShooting(animation_controller, transform);
		UpdateDirection(transform);

		switch (state)
		{
		case State::Idle:
			ProcessIdle(animation_controller, transform);
			break;

		case State::Move:
			ProcessMovement(animation_controller, transform);
			break;
		}

		UpdateMovementWeights();
	}

	void CharacterController::ProcessIdle(components::AnimationController* animation_controller, components::Transform* transform)
	{
		const auto move_length = glm::length(input.move_direction);
		const auto aim_length = glm::length(input.aim_direction);
		if (move_length > 0.01f)
		{
			state = State::Move;
			animation_controller->mixer->FadeOutAllAnimations(0.3f, STATIONARY_ANIM_LAYER);

			bool should_reset = true;
			for (auto& handle : move_playback_handles)
				if (handle.GetWeight() > 0.01)
				{
					should_reset = false;
					break;
				}
			
			if (should_reset)
				ResetMovePlayback();

			ProcessMovement(animation_controller, transform);
		}
	}

	void CharacterController::ProcessMovement(components::AnimationController* animation_controller, components::Transform* transform)
	{
		const auto move_length = glm::length(input.move_direction);
		const auto aim_length = glm::length(input.aim_direction);

		if (move_length < 0.01f)
		{
			state = State::Idle;
			const auto stationary_params = SkeletalAnimation::PlaybackParams().Playback(SkeletalAnimation::PlaybackMode::Loop).FadeTime(0.3f).Layer(STATIONARY_ANIM_LAYER);
			animation_controller->mixer->PlayAnimation(
				animations.stationary[StationaryAnimationType::Idle],
				stationary_params
			);

			ClearMoveTargets();
		}
		else
		{
			ClearMoveTargets();

			const auto move_dir = input.move_direction / move_length;
			const float dot = glm::clamp(glm::dot(current_aim_dir, move_dir), -1.0f, 1.0f);
			const float angle = acosf(dot);
			const bool direction = glm::cross(vec3(current_aim_dir, 0), vec3(move_dir, 0)).z > 0;

			const float float_index = angle / ((float)M_PI / 4.0f);
			const uint32_t indexA = glm::min((uint32_t)float_index, 3u);
			const uint32_t indexB = indexA + 1;
			assert(indexA < 4);
			const float weightB = (float_index - (float)indexA);
			const float weightA = 1 - weightB;

			assert(weightA >= 0 && weightA <= 1.0f);
			assert(weightB >= 0 && weightB <= 1.0f);

			const uint32_t index_shift = direction ? 0 : 5;
			move_target_weights[RUN_INDICES[indexA + index_shift]] = weightA;
			move_target_weights[RUN_INDICES[indexB + index_shift]] = weightB;


			//auto debug_draw = Engine::Get()->GetDebugDraw();
			//debug_draw->DrawLine(transform->position, transform->position + vec3(move_dir.x, 0, move_dir.y), vec4(0, 0, 1, 1));
		}
	}

	void CharacterController::UpdateDirection(components::Transform* transform)
	{
		const auto aim_length = glm::length(input.aim_direction);
		if (aim_length < 0.001f)
			return;

		const auto aim_dir = input.aim_direction / aim_length;
		const float dot = glm::dot(current_aim_dir, aim_dir);
		const float angle = acosf(dot);
		const float direction = glm::cross(vec3(current_aim_dir, 0), vec3(aim_dir, 0)).z > 0 ? 1 : -1;

		auto delta_time = GetManager().GetStaticComponent<components::DeltaTime>();
		const float angle_delta = std::min(delta_time->dt * (float)M_PI, angle) * direction;

		current_aim_dir = glm::normalize(glm::rotate(current_aim_dir, angle_delta));

		auto rotation_angle = acosf(glm::dot(vec2(0, 1), current_aim_dir));
		const float rotation_direction = glm::cross(vec3(0, 1, 0), vec3(current_aim_dir, 0)).z > 0 ? 1 : -1;

		transform->rotation = glm::angleAxis(rotation_angle, vec3(0, -rotation_direction, 0));

		//auto debug_draw = Engine::Get()->GetDebugDraw();
		//debug_draw->DrawLine(transform->position, transform->position + vec3(current_aim_dir.x, 0, current_aim_dir.y), vec4(0,1,0,1));
		//debug_draw->DrawLine(transform->position, transform->position + vec3(aim_dir.x, 0, aim_dir.y), vec4(1, 0, 0, 1));
	}

	void CharacterController::UpdateMovementWeights()
	{
		auto delta_time = GetManager().GetStaticComponent<components::DeltaTime>();
		//const auto move_length = glm::length(input.move_direction);
		//const float speed = move_length ? 4.0 : 4.0;
		const float speed = 4.0f;

		for (uint32_t i = 0; i < move_playback_handles.size(); i++)
		{
			auto& handle = move_playback_handles[i];
			const float current_weight = handle.GetWeight();
			const float target_weight = move_target_weights[i];
			float weight_delta = target_weight - current_weight;
			const bool positive = weight_delta > 0;
			weight_delta = (positive ? 1.0f : -1.0f) * speed * delta_time->dt;

			float new_weight = current_weight + weight_delta;
			if (positive)
				new_weight = std::min(target_weight, new_weight);
			else
				new_weight = std::max(target_weight, new_weight);

			handle.SetWeight(new_weight);
		}
	}

}