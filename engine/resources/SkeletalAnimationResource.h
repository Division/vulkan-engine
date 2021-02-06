#pragma once

#include "ResourceCache.h"
#include "render/device/Resource.h"
#include "ozz/animation/runtime/animation.h"

namespace Resources
{

	class SkeletalAnimationResource
	{
	public:
		using Handle = Handle<SkeletalAnimationResource>;

		SkeletalAnimationResource(const std::wstring& filename);
		~SkeletalAnimationResource();

		const ozz::animation::Animation* Get() const { return animation.get(); }

	private:
		std::unique_ptr<ozz::animation::Animation> animation;
	};

}