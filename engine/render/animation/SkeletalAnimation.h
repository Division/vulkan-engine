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

namespace SkeletalAnimation
{
	enum class PlaybackMode
	{
		Once,
		Loop,
		Clamp
	};

	constexpr float DEFAULT_FADE_TIME = 0.3f;

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

			bool SetSpeed(float speed)
			{
				if (auto instance = weak_reference.lock())
					instance->SetSpeed(speed);
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

		AnimationInstance(Resources::SkeletonResource::Handle skeleton, Resources::SkeletalAnimationResource::Handle animation, PlaybackMode playback_mode, uint32_t layer);

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

		auto& GetCache() { return cache; }
		auto& GetLocals() { return locals; }

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

		bool is_playing = false;
		float progress = 0; // Progress of animation in range 0..1
		float weight = 1.0f;
		float speed = 1.0f;
		bool finished = false;
		uint32_t layer = 0;
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
		AnimationInstance::Handle PlayAnimation(Resources::SkeletalAnimationResource::Handle animation, PlaybackMode playback_mode = PlaybackMode::Once, float fade_duration = DEFAULT_FADE_TIME, uint32_t layer = 0);
		AnimationInstance::Handle BlendAnimation(Resources::SkeletalAnimationResource::Handle animation, PlaybackMode playback_mode = PlaybackMode::Once, float fade_duration = DEFAULT_FADE_TIME, uint32_t layer = 0);
		Resources::SkeletonResource::Handle GetSkeleton() const { return skeleton; }

		const ozz::span<const ozz::math::Float4x4> GetModelMatrices() const { return ozz::make_span(model_matrices); }

	private:
		Resources::SkeletonResource::Handle skeleton;
		std::vector<std::shared_ptr<AnimationInstance>> instances;
		std::vector<ozz::animation::BlendingJob::Layer> blend_layers;

		// Buffer of local transforms which stores the blending result.
		ozz::vector<ozz::math::SoaTransform> blended_locals;

		// Buffer of model space matrices. These are computed by the local-to-model
		// job after the blending stage.
		ozz::vector<ozz::math::Float4x4> model_matrices;
	};
}