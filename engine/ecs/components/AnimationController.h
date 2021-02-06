#pragma once

#include "render/animation/SkeletalAnimation.h"

namespace ECS::components
{

	struct AnimationController
	{
		AnimationController(Resources::SkeletonResource::Handle skeleton)
			: mixer(std::make_unique<SkeletalAnimation::AnimationMixer>(skeleton))
		{}

		AnimationController(AnimationController&& other) = default;

		std::unique_ptr<SkeletalAnimation::AnimationMixer> mixer;
	};

}

