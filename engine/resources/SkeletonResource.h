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
		const std::vector<glm::mat4x4>& GetBindposeMatrices() const { return bindpose_matrices; };
		const std::vector<glm::mat4x4>& GetInverseBindposeMatrices() const { return inv_bindpose_matrices; };

	private:
		std::unique_ptr<ozz::animation::Skeleton> skeleton;
		std::vector<glm::mat4x4> bindpose_matrices;
		std::vector<glm::mat4x4> inv_bindpose_matrices;
	};

}