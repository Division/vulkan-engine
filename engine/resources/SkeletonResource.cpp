#include "SkeletonResource.h"
#include "loader/FileLoader.h"
#include "ozz/base/io/stream.h"
#include "ozz/base/io/archive.h"
#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/skeleton.h"

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
	}
	
	SkeletonResource::~SkeletonResource()
	{

	}

}