#pragma once

#include <mutex>
#include <atomic>
#include <array>
#include <vector>

namespace Common
{
	class Resource
	{
	public:
		virtual ~Resource() = default;
	private:
	};

	class ResourceReleaser
	{
	public:
		static constexpr uint32_t bucket_count = 2;

		void Add(std::shared_ptr<Resource> resource);
		void Swap();
		size_t Clear();

	private:
		size_t ClearBucket(int index);

		std::recursive_mutex mutex;
		uint32_t current_bucket = 0;
		std::array<std::vector<std::shared_ptr<Resource>>, bucket_count> buckets;
	};

	ResourceReleaser& GetReleaser();

	#include "Handle.inl"
}