#include "SkeletalAnimationResource.h"
#include "loader/FileLoader.h"
#include "ozz/base/io/stream.h"
#include "ozz/base/io/archive.h"
#include "ozz/animation/runtime/animation.h"

namespace Resources
{

	SkeletalAnimationResource::SkeletalAnimationResource(const std::wstring& filename)
	{
		auto data = loader::LoadFile(filename);

		if (!data.size())
			throw Exception(filename) << "Error reading skeletal animation resource";

		ozz::io::MemoryStream stream;
		stream.Write(data.data(), data.size());
		stream.Seek(0, ozz::io::Stream::kSet);

		ozz::io::IArchive archive(&stream);

		if (!archive.TestTag<ozz::animation::Animation>())
			throw Exception(filename) << "Error reading skeletal animation resource";

		auto animation_temp = std::make_unique<ozz::animation::Animation>();
		archive >> *animation_temp;

		animation = std::move(animation_temp);
	}

	SkeletalAnimationResource::~SkeletalAnimationResource()
	{

	}

}