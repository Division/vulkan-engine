#pragma once

#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/blending_job.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/base/log.h"
#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/vec_float.h"
#include "ozz/options/options.h"
#include "ozz/base/containers/vector.h"
#include "ozz/base/span.h"
#include "resources/SkeletalAnimationResource.h"
#include "resources/SkeletonResource.h"
#include "utils/DataStructures.h"
#include <magic_enum/magic_enum.hpp>

namespace SkeletalAnimation
{
	enum class PlaybackMode
	{
		Once,
		Loop,
		Clamp
	};

	enum class BlendingMode
	{
		Normal,
		Additive
	};

	constexpr float DEFAULT_FADE_TIME = 0.3f;

	struct PlaybackParams
	{
		PlaybackMode playback_mode = PlaybackMode::Once;
		BlendingMode blending_mode = BlendingMode::Normal;
		float fade_time = DEFAULT_FADE_TIME;
		uint32_t layer = 0;

		PlaybackParams& Playback(PlaybackMode value) { playback_mode = value; return *this; }
		PlaybackParams& Blending(BlendingMode value) { blending_mode = value; return *this; }
		PlaybackParams& FadeTime(float value) { fade_time = value; return *this; }
		PlaybackParams& Layer(uint32_t value) { layer = value; return *this; }
	};

	class AnimationInstance
	{
	public:
		class Handle
		{
		public:
			Handle(std::shared_ptr<AnimationInstance> instance) : weak_reference(instance) {}
			Handle() = default;
			Handle(const Handle&) = default;
			Handle(Handle&&) = default;
			Handle& operator=(Handle&&) = default;
			Handle& operator=(const Handle&) = default;
			Handle& operator=(std::nullptr_t) { weak_reference.reset(); return *this; };

			operator bool() const { return !weak_reference.expired(); }

			bool Play()
			{
				if (auto instance = weak_reference.lock())
					instance->Play();
				else
					return false;

				return true;
			}

			bool Pause()
			{
				if (auto instance = weak_reference.lock())
					instance->Pause();
				else
					return false;

				return true;
			}

			bool SetProgress(float progress)
			{
				if (auto instance = weak_reference.lock())
					instance->SetProgress(progress);
				else
					return false;

				return true;
			}

			bool SetWeight(float weight) 
			{
				if (auto instance = weak_reference.lock())
					instance->SetWeight(weight);
				else
					return false;

				return true;
			}

			float GetWeight()
			{
				if (auto instance = weak_reference.lock())
					return instance->GetWeight();
				else
					return 0;
			}

			bool SetSpeed(float speed)
			{
				if (auto instance = weak_reference.lock())
					instance->SetSpeed(speed);
				else
					return false;

				return true;
			}

			bool FadeOut(float fade_duration_seconds, bool finish = true)
			{
				if (auto instance = weak_reference.lock())
					instance->FadeOut(fade_duration_seconds, finish);
				else
					return false;

				return true;
			}

		private:
			std::weak_ptr<AnimationInstance> weak_reference;
		};

		struct BlendEvent
		{
			enum class Type { Blend, Finish };
			float start_progress;
			float duration_progress;
			float start_value;
			float target_value;
			Type type = Type::Blend;
		};

		AnimationInstance(Resources::SkeletonResource::Handle skeleton, Resources::SkeletalAnimationResource::Handle animation, const PlaybackParams& params);

		void Play()
		{
			is_playing = true;
		}

		void Pause()
		{
			is_playing = false;
		}

		bool GetIsPlaying() const { return is_playing; }

		void SetProgress(float progress)
		{
			this->progress = std::min(std::max(0.0f, progress), 1.0f);
			if (root_motion_enabled)
				should_skip_root_update = true;
		}

		float GetProgress() const { return progress; }

		void SetWeight(float weight)
		{
			this->weight = weight;
		}

		float GetWeight() const { return weight; }

		void SetSpeed(float speed)
		{
			this->speed = speed;
		}

		float GetSpeed() const { return speed; }

		bool IsFinished() const { return finished; }

		uint32_t GetLayer() const { return layer; }

		const auto& GetAnimation() { return animation; }
		void Update(float dt);
		void RemoveBlendEvents();

		void FadeIn(float fade_duration_seconds);
		void FadeOut(float fade_duration_seconds, bool finish = true);
		void AddBlendEvent(BlendEvent::Type type, float start, float duration = 0, float start_value = 0, float target_value = 0);
		BlendingMode GetBlendingMode() const { return blend_mode; }

		auto& GetCache() { return cache; }
		auto& GetLocals() { return locals; }
		vec3 GetLastRootPosition() const { return last_root_position; }
		void SetLastRootPosition(vec3 value) { last_root_position = value; }
		bool GetShouldSkipRootUpdate() const { return should_skip_root_update; }
		void SetShouldSkipRootUpdate(bool value) { should_skip_root_update = value; }

		bool GetRootMotionEnabled() const { return root_motion_enabled; }
		void SetRootMotionEnabled(bool value) { root_motion_enabled = value; }

	private:
		void UpdateBlendEvents(float progress);

	private:
		Resources::SkeletonResource::Handle skeleton;
		Resources::SkeletalAnimationResource::Handle animation_resource;
		const ozz::animation::Animation& animation;
		const float duration;
		const PlaybackMode playback_mode;

		// Sampling cache.
		ozz::animation::SamplingCache cache;

		// Buffer of local transforms as sampled from animation.
		ozz::vector<ozz::math::SoaTransform> locals;

		BlendingMode blend_mode = BlendingMode::Normal;
		bool is_playing = false;
		float progress = 0; // Progress of animation in range 0..1
		float weight = 1.0f;
		float speed = 1.0f;
		bool finished = false;
		uint32_t layer = 0;

		bool should_skip_root_update = true;
		bool root_motion_enabled = false;
		uint32_t root_motion_bone_index = 0;
		vec3 last_root_position = vec3(0);
		utils::SmallVector<BlendEvent, 4> blend_events;
	};

	class AnimationMixer
	{
	public:
		AnimationMixer(Resources::SkeletonResource::Handle skeleton);
		~AnimationMixer();

		void Update(float dt);
		void ProcessBlending();
		void FadeOutAllAnimations(float fade_duration, uint32_t layer);
		AnimationInstance::Handle PlayAnimation(Resources::SkeletalAnimationResource::Handle animation, const PlaybackParams& params = {});
		AnimationInstance::Handle BlendAnimation(Resources::SkeletalAnimationResource::Handle animation, const PlaybackParams& params = {});
		Resources::SkeletonResource::Handle GetSkeleton() const { return skeleton; }

		const ozz::span<const ozz::math::Float4x4> GetModelMatrices() const { return ozz::make_span(model_matrices); }
		const vec3& GetRootOffset() const { return root_offset; };
		void SetRootMotionEnabled(bool value) { root_motion = value; }
		bool GetRootMotionEnabled() const { return root_motion; }

	private:
		Resources::SkeletonResource::Handle skeleton;
		std::vector<std::shared_ptr<AnimationInstance>> instances;
		std::array<std::vector<ozz::animation::BlendingJob::Layer>, magic_enum::enum_count<BlendingMode>()> blend_layers;

		// Buffer of local transforms which stores the blending result.
		ozz::vector<ozz::math::SoaTransform> blended_locals;

		// Buffer of model space matrices. These are computed by the local-to-model
		// job after the blending stage.
		ozz::vector<ozz::math::Float4x4> model_matrices;

		bool root_motion = false;
		uint32_t root_motion_bone = 0;
		vec3 root_offset = vec3(0);
	};
}