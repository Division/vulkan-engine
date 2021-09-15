#pragma once

#include "ResourceCache.h"
#include "render/device/Resource.h"
#include "ozz/animation/runtime/skeleton.h"
#include "utils/StringUtils.h"

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

		uint32_t GetJointIndex(const char* joint_name) const
		{
			auto it = bone_name_to_index_map.find(joint_name);
			if (it == bone_name_to_index_map.end())
				return -1;
			else
				return it->second;
		}

		const char* GetJointName(uint32_t index) const 
		{
			if (index >= skeleton->num_joints())
				return nullptr;
			return skeleton->joint_names()[index];
		}

	private:
		std::unordered_map<const char*, uint32_t, utils::HasherChar, utils::HasherChar> bone_name_to_index_map;
		std::unique_ptr<ozz::animation::Skeleton> skeleton;
		std::vector<glm::mat4x4> bindpose_matrices;
		std::vector<glm::mat4x4> inv_bindpose_matrices;
	};

}