#include "SkeletalAnimation.h"

namespace SkeletalAnimation
{
	AnimationInstance::AnimationInstance(Resources::SkeletonResource::Handle skeleton, Resources::SkeletalAnimationResource::Handle animation, PlaybackMode playback_mode, uint32_t layer)
		: skeleton(skeleton)
		, animation_resource(animation)
		, animation(*animation->Get())
		, duration(animation->Get()->duration())
		, playback_mode(playback_mode)
		, layer(layer)
	{
		const int num_joints = skeleton->Get()->num_joints();
		const int num_soa_joints = skeleton->Get()->num_soa_joints();
		locals.resize(num_soa_joints);
		cache.Resize(num_joints);
	}

	void AnimationInstance::Update(float dt)
	{
		progress += speed * dt / duration;

		UpdateBlendEvents(progress);

		switch (playback_mode)
		{
			case PlaybackMode::Loop:
			{
				if (progress >= 1.0f)
				{
					progress = fmod(progress, 1.0f);
				}
				break;
			}
			case PlaybackMode::Once:
			case PlaybackMode::Clamp:
			{
				progress = std::min(progress, 1.0f);
				break;
			}
		}
	}

	void AnimationInstance::UpdateBlendEvents(float progress)
	{
		// Update weight
		for (auto& event : blend_events)
		{
			if (progress > event.start_progress)
			{
				if (event.type == BlendEvent::Type::Blend)
				{
					auto event_progress = std::min((progress - event.start_progress) / event.duration_progress, 1.0f);
					weight = std::min(std::max(0.0f, event.start_value + (event.target_value - event.start_value) * event_progress), 1.0f);
				}
				else if (event.type == BlendEvent::Type::Finish)
				{
					if (progress >= event.start_progress + event.duration_progress)
						finished = true;
				}
			}
		}

		// Delete finished events
		for (int i = blend_events.size() - 1; i >= 0 ; i--)
		{
			if (progress >= blend_events[i].start_progress + blend_events[i].duration_progress)
			{
				blend_events[i] = blend_events[blend_events.size() - 1];
				blend_events.pop_back();
			}
		}
	}

	void AnimationInstance::FadeIn(float fade_duration_seconds)
	{
		AddBlendEvent(BlendEvent::Type::Blend, progress, fade_duration_seconds / duration, weight, 1.0f);
	}

	void AnimationInstance::FadeOut(float fade_duration_seconds, bool finish)
	{
		AddBlendEvent(BlendEvent::Type::Blend, progress, fade_duration_seconds / duration, weight, 0.0f);
		if (finish)
			AddBlendEvent(BlendEvent::Type::Finish, progress + fade_duration_seconds / duration);
	}

	void AnimationInstance::RemoveBlendEvents()
	{
		blend_events.clear();
	}

	void AnimationInstance::AddBlendEvent(BlendEvent::Type type, float start, float duration, float start_value, float target_value)
	{
		BlendEvent event;
		event.type = type;
		event.start_progress = start;
		event.duration_progress = duration;
		event.start_value = start_value;
		event.target_value = target_value;
		blend_events.push_back(event);
	}

	AnimationMixer::AnimationMixer(Resources::SkeletonResource::Handle skeleton)
		: skeleton(skeleton)
	{
		const int num_joints = skeleton->Get()->num_joints();
		const int num_soa_joints = skeleton->Get()->num_soa_joints();

		blended_locals.resize(num_soa_joints);

		// Allocates model space runtime buffers of blended data.
		model_matrices.resize(num_joints);
	}

	AnimationMixer::~AnimationMixer() = default;

	void AnimationMixer::Update(float dt)
	{
		for (auto& instance : instances)
		{
			if (instance->IsFinished()) continue;
			instance->Update(dt);
		}

		instances.erase(
			std::remove_if(instances.begin(), instances.end(), [](const auto& instance) { return instance->IsFinished(); }),
			instances.end()
		);
	}

	void AnimationMixer::FadeOutAllAnimations(float fade_duration, uint32_t layer)
	{
		for (auto& instance : instances)
		{
			if (instance->GetLayer() != layer) continue;

			instance->RemoveBlendEvents();
			instance->FadeOut(fade_duration, true);
		}
	}

	AnimationInstance::Handle AnimationMixer::PlayAnimation(Resources::SkeletalAnimationResource::Handle animation, PlaybackMode playback_mode, float fade_duration, uint32_t layer)
	{
		FadeOutAllAnimations(fade_duration, layer);
		return BlendAnimation(animation, playback_mode, fade_duration, layer);
	}

	AnimationInstance::Handle AnimationMixer::BlendAnimation(Resources::SkeletalAnimationResource::Handle animation, PlaybackMode playback_mode, float fade_duration, uint32_t layer)
	{
		auto instance = std::make_shared<AnimationInstance>(skeleton, animation, playback_mode, layer);
		instance->FadeIn(fade_duration);

		if (playback_mode == PlaybackMode::Once)
		{
			const float anim_duration = animation->Get()->duration();
			const float fade_duration_progress = fade_duration / anim_duration;
			instance->AddBlendEvent(AnimationInstance::BlendEvent::Type::Blend, 1.0f - fade_duration_progress, fade_duration_progress, 1.0f, 0.0f);
			instance->AddBlendEvent(AnimationInstance::BlendEvent::Type::Finish, 1.0f);
		}

		instances.push_back(instance);
		return AnimationInstance::Handle(instance);
	}

	void AnimationMixer::ProcessBlending()
	{
		for (auto& instance : instances)
		{
			ozz::animation::SamplingJob sampling_job;
			sampling_job.animation = &instance->GetAnimation();
			sampling_job.cache = &instance->GetCache();
			sampling_job.ratio = instance->GetProgress();
			sampling_job.output = ozz::make_span(instance->GetLocals());

			if (!sampling_job.Run())
				throw std::runtime_error("Error sampling animation");
		}

		blend_layers.resize(instances.size());
		for (int i = 0; i < instances.size(); i++)
		{
			blend_layers[i].transform = ozz::make_span(instances[i]->GetLocals());
			blend_layers[i].weight = instances[i]->GetWeight();
		}

		// Setups blending job.
		ozz::animation::BlendingJob blend_job;
		blend_job.threshold = 0.1f;
		blend_job.layers = ozz::make_span(blend_layers);
		blend_job.bind_pose = skeleton->Get()->joint_bind_poses();
		blend_job.output = make_span(blended_locals);

		// Blends.
		if (!blend_job.Run())
			throw std::runtime_error("Error blending animation");

		// Converts from local space to model space matrices.
		// Gets the output of the blending stage, and converts it to model space.

		// Setup local-to-model conversion job.
		ozz::animation::LocalToModelJob ltm_job;
		ltm_job.skeleton = skeleton->Get();
		ltm_job.input = make_span(blended_locals);
		ltm_job.output = make_span(model_matrices);

		// Runs ltm job.
		if (!ltm_job.Run())
			throw std::runtime_error("Error calculating model matrices");
	}
}