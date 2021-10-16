#include "SkeletalAnimation.h"
//#include <debugapi.h>

namespace SkeletalAnimation
{
	AnimationInstance::AnimationInstance(uint64_t id, Resources::SkeletonResource::Handle skeleton, Resources::SkeletalAnimationResource::Handle animation, const PlaybackParams& params)
		: id(id)
		, skeleton(skeleton)
		, animation_resource(animation)
		, animation(*animation->Get())
		, duration(animation->Get()->duration())
		, playback_mode(params.playback_mode)
		, blend_mode(params.blending_mode)
		, layer(params.layer)
	{
		const int num_joints = skeleton->Get()->num_joints();
		const int num_soa_joints = skeleton->Get()->num_soa_joints();
		locals.resize(num_soa_joints);
		cache.Resize(num_joints);
	}

	void AnimationInstance::Update(float dt, Dispatcher& dispatcher)
	{
		progress += speed * dt / duration;

		UpdateBlendEvents(progress);

		switch (playback_mode)
		{
			case PlaybackMode::Loop:
			{
				if (progress >= 1.0f)
				{
					if (progress >= 1.0f)
						dispatcher.Dispatch(EventType::Loop, EventParam{ id });

					progress = fmod(progress, 1.0f);
					if (GetRootMotionEnabled())
					{
						last_root_position = vec3(0);
					}
				}
				break;
			}
			case PlaybackMode::Once:
			case PlaybackMode::Clamp:
			{
				if (progress >= 1.0f)
					dispatcher.Dispatch(EventType::Complete, EventParam{ id });

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
			instance->Update(dt, dispatcher);
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

	AnimationInstance::Handle AnimationMixer::PlayAnimation(Resources::SkeletalAnimationResource::Handle animation, const PlaybackParams& params)
	{
		FadeOutAllAnimations(params.fade_time, params.layer);
		return BlendAnimation(animation, params);
	}

	AnimationInstance::Handle AnimationMixer::BlendAnimation(Resources::SkeletalAnimationResource::Handle animation, const PlaybackParams& params)
	{
		auto instance = std::make_shared<AnimationInstance>(++instance_counter, skeleton, animation, params);
		instance->SetWeight(0);
		instance->FadeIn(params.fade_time);

		dispatcher.Dispatch(EventType::Start, EventParam{ instance->GetID() });

		if (params.playback_mode == PlaybackMode::Once)
		{
			const float anim_duration = animation->Get()->duration();
			const float fade_duration_progress = params.fade_time / anim_duration;
			instance->AddBlendEvent(AnimationInstance::BlendEvent::Type::Blend, 1.0f - fade_duration_progress, fade_duration_progress, 1.0f, 0.0f);
			instance->AddBlendEvent(AnimationInstance::BlendEvent::Type::Finish, 1.0f);
		}

		if (root_motion)
			instance->SetRootMotionEnabled(true);

		instances.push_back(instance);
		return AnimationInstance::Handle(instance);
	}

	vec3 RemoveTranslation(uint32_t _index, const ozz::span<ozz::math::SoaTransform>& _transforms) {
		assert(_index < _transforms.size() * 4 && "joint index out of bound.");

		ozz::math::SimdFloat4 translations[4];
		ozz::math::SoaTransform& bind_pose = _transforms[_index / 4];
		ozz::math::Transpose3x4(&bind_pose.translation.x, translations);
		ozz::math::SimdFloat4& translation = translations[_index & 3];
		const vec4 v_translation = (vec4&)translation;
		const vec3 result(v_translation.x, 0, v_translation.z);
		translation = ozz::math::simd_float4::Load(0, v_translation.y, 0, 1); // remove xz translation from the transform
		ozz::math::Transpose4x3(translations, &bind_pose.translation.x);

		return result;
	}

	void AnimationMixer::ProcessBlending()
	{
		for (auto &layer : blend_layers)
			layer.clear();

		root_offset = vec3(0);

		std::array<float, magic_enum::enum_count<BlendingMode>()> total_weight;
		total_weight.fill(0.0001f);

		for (auto& instance : instances)
			total_weight[(uint32_t)instance->GetBlendingMode()] += instance->GetWeight();

		for (auto& instance : instances)
		{
			if (instance->GetWeight() < 0.001f)
			{
				instance->SetShouldSkipRootUpdate(true);
				continue;
			}

			ozz::animation::SamplingJob sampling_job;
			sampling_job.animation = &instance->GetAnimation();
			sampling_job.cache = &instance->GetCache();
			sampling_job.ratio = instance->GetProgress();
			sampling_job.output = ozz::make_span(instance->GetLocals());

			if (!sampling_job.Run())
				throw std::runtime_error("Error sampling animation");

			const uint32_t layer_index = (uint32_t)instance->GetBlendingMode();

			auto& layer = blend_layers[layer_index].emplace_back();
			layer.weight = instance->GetWeight() / total_weight[layer_index];
			layer.transform = ozz::make_span(instance->GetLocals());

			if (instance->GetRootMotionEnabled())
			{
				const auto offset = RemoveTranslation(root_motion_bone, make_span(sampling_job.output));
				if (!instance->GetShouldSkipRootUpdate())
				{
					root_offset += (offset - instance->GetLastRootPosition()) * layer.weight;
				}

				instance->SetShouldSkipRootUpdate(false);
				instance->SetLastRootPosition(offset);
			}
		}

		// Setups blending job.
		ozz::animation::BlendingJob blend_job;
		blend_job.threshold = 0.1f;
		blend_job.layers = ozz::make_span(blend_layers[(uint32_t)BlendingMode::Normal]);
		blend_job.additive_layers = ozz::make_span(blend_layers[(uint32_t)BlendingMode::Additive]);
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

		UpdateSockets();
	}

	const mat4* AnimationMixer::GetModelMatrix(uint32_t index) const
	{
		if (index < SOCKET_OFFSET)
			return index < model_matrices.size() ? (mat4*)&model_matrices[index] : nullptr;
		else
			return index - SOCKET_OFFSET < sockets.size() ? &sockets[index - SOCKET_OFFSET].GetModelMatrix() : nullptr;
	}

	AnimationMixer::Socket* AnimationMixer::AddSocket(const char* name, uint32_t parent_index)
	{
		auto socket = GetSocket(name);
		if (!socket)
		{
			socket = &sockets.emplace_back(name, parent_index);
		}

		socket->parent_bone_index = parent_index;

		return socket;
	}

	void AnimationMixer::RemoveSocket(const char* name)
	{
		const uint32_t hash = FastHash(name);
		sockets.erase(std::remove_if(sockets.begin(), sockets.end(), [hash](const Socket& socket) { return socket.GetNameHash() == hash; }));
	}

	const AnimationMixer::Socket* AnimationMixer::GetSocket(const char* name) const
	{
		return GetSocket(name);
	}

	AnimationMixer::Socket* AnimationMixer::GetSocket(const char* name)
	{
		const uint32_t hash = FastHash(name);
		auto it = std::find_if(sockets.begin(), sockets.end(), [hash](const Socket& socket) { return socket.GetNameHash() == hash; });
		return it != sockets.end() ? &*it : nullptr;
	}

	std::optional<uint32_t> AnimationMixer::GetSocketIndex(const char* name) const
	{
		const uint32_t hash = FastHash(name);
		auto it = std::find_if(sockets.begin(), sockets.end(), [hash](const Socket& socket) { return socket.GetNameHash() == hash; });
		if (it == sockets.end())
			return std::nullopt;
		else
			return SOCKET_OFFSET + std::distance(sockets.begin(), it);
	}

	const mat4& AnimationMixer::Socket::GetLocalMatrix() const
	{
		if (dirty)
		{
			matrix = ComposeMatrix(position, rotation, scale);
			dirty = false;
		}

		return matrix;
	}

	void AnimationMixer::UpdateSockets()
	{
		for (auto& socket : sockets)
		{
			auto parent_matrix = (mat4&)GetModelMatrices()[socket.GetParentBone()];
			socket.local_to_model = parent_matrix * socket.GetLocalMatrix();
		}
	}
}