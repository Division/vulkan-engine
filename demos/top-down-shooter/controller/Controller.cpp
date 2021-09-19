#include "Controller.h"
#include "ecs/components/AnimationController.h"
#include "ecs/components/Transform.h"
#include "ecs/components/Static.h"
#include <glm/gtx/rotate_vector.hpp>

#include "Engine.h"
#include "render/debug/DebugDraw.h"

namespace ECS::components
{
	void CharacterController::Startup(AnimationController* animation_controller)
	{
		const auto move_params = SkeletalAnimation::PlaybackParams().Playback(SkeletalAnimation::PlaybackMode::Loop).FadeTime(0.0f).Layer(MOVE_ANIM_LAYER);

		// move animations are always active and manipulated by handles. Zero weight animation has zero cost.
		for (uint32_t i = 0; i < move_animations.size(); i++)
		{
			move_playback_handles[i] = animation_controller->mixer->BlendAnimation(move_animations[i], move_params);
			move_playback_handles[i].SetWeight(0);
		}

		move_target_weights.fill(0);

		const auto stationary_params = SkeletalAnimation::PlaybackParams().Playback(SkeletalAnimation::PlaybackMode::Loop).FadeTime(0.0f).Layer(STATIONARY_ANIM_LAYER);
		animation_controller->mixer->PlayAnimation(stationary_animations[StationaryAnimationType::Idle], stationary_params);
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

}

namespace ECS::systems
{

	void CharacterControllerSystem::Process(Chunk* chunk)
	{
		ComponentFetcher<components::CharacterController> character_controller_fetcher(*chunk);
		ComponentFetcher<components::AnimationController> animation_controller_fetcher(*chunk);
		ComponentFetcher<components::Transform> transform_fetcher(*chunk);

		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{
			auto* character_controller = character_controller_fetcher.GetComponent(i);
			auto* animation_controller = animation_controller_fetcher.GetComponent(i);
			auto* transform = transform_fetcher.GetComponent(i);
			ProcessController(character_controller, animation_controller, transform);
		}
	}

	void CharacterControllerSystem::ProcessShooting(components::CharacterController* character_controller, components::AnimationController* animation_controller, components::Transform* transform)
	{
		if (character_controller->input.shoot && !character_controller->shoot_animation_handle)
		{
			const auto params = SkeletalAnimation::PlaybackParams()
									.Playback(SkeletalAnimation::PlaybackMode::Loop)
									.Blending(SkeletalAnimation::BlendingMode::Additive)
									.FadeTime(0.15f).Layer(components::CharacterController::ADDITIVE_LAYER);
			auto& animation = character_controller->stationary_animations[components::CharacterController::StationaryAnimationType::Shoot];
			character_controller->shoot_animation_handle = animation_controller->mixer->PlayAnimation(animation, params);
		}
		else if (!character_controller->input.shoot && character_controller->shoot_animation_handle)
		{
			character_controller->shoot_animation_handle.FadeOut(0.15);
			character_controller->shoot_animation_handle = nullptr;
		}
	}

	void CharacterControllerSystem::ProcessController(components::CharacterController* character_controller, components::AnimationController* animation_controller, components::Transform* transform)
	{
		ProcessShooting(character_controller, animation_controller, transform);
		UpdateDirection(character_controller, transform);

		switch (character_controller->state)
		{
		case components::CharacterController::State::Idle:
			ProcessIdle(character_controller, animation_controller, transform);
			break;
		

		case components::CharacterController::State::Move:
			ProcessMovement(character_controller, animation_controller, transform);
			break;
		}

		UpdateMovementWeights(character_controller);
	}

	void CharacterControllerSystem::ProcessIdle(components::CharacterController* character_controller, components::AnimationController* animation_controller, components::Transform* transform)
	{
		auto& input = character_controller->input;
		const auto move_length = glm::length(input.move_direction);
		const auto aim_length = glm::length(input.aim_direction);
		if (move_length > 0.01f)
		{
			character_controller->state = components::CharacterController::State::Move;
			animation_controller->mixer->FadeOutAllAnimations(0.3f, components::CharacterController::STATIONARY_ANIM_LAYER);

			bool should_reset = true;
			for (auto& handle : character_controller->move_playback_handles)
				if (handle.GetWeight() > 0.01)
				{
					should_reset = false;
					break;
				}
			
			if (should_reset)
				character_controller->ResetMovePlayback();

			ProcessMovement(character_controller, animation_controller, transform);
		}
	}

	void CharacterControllerSystem::ProcessMovement(components::CharacterController* character_controller, components::AnimationController* animation_controller, components::Transform* transform)
	{
		auto& input = character_controller->input;
		const auto move_length = glm::length(input.move_direction);
		const auto aim_length = glm::length(input.aim_direction);

		if (move_length < 0.01f)
		{
			character_controller->state = components::CharacterController::State::Idle;
			const auto stationary_params = SkeletalAnimation::PlaybackParams().Playback(SkeletalAnimation::PlaybackMode::Loop).FadeTime(0.3f).Layer(components::CharacterController::STATIONARY_ANIM_LAYER);
			animation_controller->mixer->PlayAnimation(
				character_controller->stationary_animations[components::CharacterController::StationaryAnimationType::Idle], 
				stationary_params
			);

			character_controller->ClearMoveTargets();
		}
		else
		{
			character_controller->ClearMoveTargets();

			const auto move_dir = input.move_direction / move_length;
			const float dot = glm::clamp(glm::dot(character_controller->current_aim_dir, move_dir), -1.0f, 1.0f);
			const float angle = acosf(dot);
			const bool direction = glm::cross(vec3(character_controller->current_aim_dir, 0), vec3(move_dir, 0)).z > 0;

			const float float_index = angle / ((float)M_PI / 4.0f);
			const uint32_t indexA = glm::min((uint32_t)float_index, 3u);
			const uint32_t indexB = indexA + 1;
			assert(indexA < 4);
			const float weightB = (float_index - (float)indexA);
			const float weightA = 1 - weightB;

			assert(weightA >= 0 && weightA <= 1.0f);
			assert(weightB >= 0 && weightB <= 1.0f);

			const uint32_t index_shift = direction ? 0 : 5;
			character_controller->move_target_weights[components::CharacterController::RUN_INDICES[indexA + index_shift]] = weightA;
			character_controller->move_target_weights[components::CharacterController::RUN_INDICES[indexB + index_shift]] = weightB;


			auto debug_draw = Engine::Get()->GetDebugDraw();
			debug_draw->DrawLine(transform->position, transform->position + vec3(move_dir.x, 0, move_dir.y), vec4(0, 0, 1, 1));
		}
	}

	void CharacterControllerSystem::UpdateDirection(components::CharacterController* character_controller, components::Transform* transform)
	{
		auto& input = character_controller->input;
		const auto aim_length = glm::length(input.aim_direction);
		if (aim_length < 0.001f)
			return;

		const auto aim_dir = input.aim_direction / aim_length;
		const float dot = glm::dot(character_controller->current_aim_dir, aim_dir);
		const float angle = acosf(dot);
		const float direction = glm::cross(vec3(character_controller->current_aim_dir, 0), vec3(aim_dir, 0)).z > 0 ? 1 : -1;

		auto delta_time = manager.GetStaticComponent<components::DeltaTime>();
		const float angle_delta = std::min(delta_time->dt * (float)M_PI, angle) * direction;

		character_controller->current_aim_dir = glm::normalize(glm::rotate(character_controller->current_aim_dir, angle_delta));

		auto rotation_angle = acosf(glm::dot(vec2(0, 1), character_controller->current_aim_dir));
		const float rotation_direction = glm::cross(vec3(0, 1, 0), vec3(character_controller->current_aim_dir, 0)).z > 0 ? 1 : -1;

		transform->rotation = glm::angleAxis(rotation_angle, vec3(0, -rotation_direction, 0));

		auto debug_draw = Engine::Get()->GetDebugDraw();
		debug_draw->DrawLine(transform->position, transform->position + vec3(character_controller->current_aim_dir.x, 0, character_controller->current_aim_dir.y), vec4(0,1,0,1));
		debug_draw->DrawLine(transform->position, transform->position + vec3(aim_dir.x, 0, aim_dir.y), vec4(1, 0, 0, 1));
	}

	void CharacterControllerSystem::UpdateMovementWeights(components::CharacterController* character_controller)
	{
		auto delta_time = manager.GetStaticComponent<components::DeltaTime>();
		auto& input = character_controller->input;
		//const auto move_length = glm::length(input.move_direction);
		//const float speed = move_length ? 4.0 : 4.0;
		const float speed = 4.0f;

		for (uint32_t i = 0; i < character_controller->move_playback_handles.size(); i++)
		{
			auto& handle = character_controller->move_playback_handles[i];
			const float current_weight = handle.GetWeight();
			const float target_weight = character_controller->move_target_weights[i];
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