#include "Handle.h"

namespace Common
{
	ResourceReleaser& GetReleaser()
	{
		static ResourceReleaser instance;
		return instance;
	}

	void ResourceReleaser::Add(std::shared_ptr<Resource> resource)
	{
		std::scoped_lock lock(mutex);
		buckets[current_bucket].push_back(resource);
	}

	void ResourceReleaser::Swap()
	{
		std::scoped_lock lock(mutex);
		current_bucket = (current_bucket + 1) % bucket_count;
		auto next_bucket = (current_bucket + 1) % bucket_count;
		buckets[next_bucket].clear();
	}

	void ResourceReleaser::Clear()
	{
		std::scoped_lock lock(mutex);

		bool has_items = true;
		do
		{
			has_items = false;

			for (auto& list : buckets)
			{
				if (!list.empty())
				{
					has_items = true;
					list.clear();
				}
			}

		} while (has_items);
	}
}