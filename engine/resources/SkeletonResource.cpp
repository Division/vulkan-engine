#include "SkeletonResource.h"
#include "loader/FileLoader.h"
#include "ozz/base/io/stream.h"
#include "ozz/base/io/archive.h"
#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_float4x4.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/span.h"
#include "ozz/base/maths/vec_float.h"
#include "ozz/base/containers/vector.h"
using namespace ozz;

namespace Resources
{

	SkeletonResource::SkeletonResource(const std::wstring& filename)
	{
		auto data = loader::LoadFile(filename);

		if (!data.size())
			throw Exception(filename) << "Error reading skeleton resource";

		ozz::io::MemoryStream stream;
		stream.Write(data.data(), data.size());
		stream.Seek(0, ozz::io::Stream::kSet);

		ozz::io::IArchive archive(&stream);

		if (!archive.TestTag<ozz::animation::Skeleton>()) 
			throw Exception(filename) << "Error reading skeleton resource";

		auto skeleton_temp = std::make_unique<ozz::animation::Skeleton>();
		archive >> *skeleton_temp;

		skeleton = std::move(skeleton_temp);

		// obtain bind pose in model space
		// TODO: move to export time
		ozz::animation::LocalToModelJob ltm_job;
		ozz::vector<ozz::math::Float4x4> model_matrices;
		model_matrices.resize(skeleton->num_joints());
		ltm_job.skeleton = skeleton.get();
		ltm_job.input = ozz::make_span(skeleton->joint_bind_poses());
		ltm_job.output = ozz::make_span(model_matrices);

		if (!ltm_job.Run())
			throw std::runtime_error("Error calculating model bindpose matrices");

		bindpose_matrices.resize(model_matrices.size());
		inv_bindpose_matrices.resize(model_matrices.size());
		for (int i = 0; i < model_matrices.size(); i++)
		{
			bindpose_matrices[i] = (mat4x4&)model_matrices[i];
			inv_bindpose_matrices[i] = glm::inverse((mat4x4&)model_matrices[i]);
		}

		for (uint32_t i = 0; i < skeleton->num_joints(); i++)
			bone_name_to_index_map[skeleton->joint_names()[i]] = i;
	}
	
	SkeletonResource::~SkeletonResource()
	{

	}

}