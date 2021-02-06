#pragma once

#include "ResourceCache.h"
#include "render/device/Resource.h"
#include "ozz/animation/runtime/skeleton.h"

namespace Resources
{

	class SkeletonResource
	{
	public:
		using Handle = Handle<SkeletonResource>;

		SkeletonResource(const std::wstring& filename);
		~SkeletonResource();

		const ozz::animation::Skeleton* Get() const { return skeleton.get(); }

	private:
		std::unique_ptr<ozz::animation::Skeleton> skeleton;
	};

}