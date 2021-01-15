ResourceReleaser& GetReleaser()
{
	static ResourceReleaser instance;
	return instance;
}

void ResourceReleaser::Add(std::shared_ptr<Resource> resource)
{
	std::scoped_lock lock(mutex);

	for (auto& list : buckets)
	{
		for (auto& r : list)
		{
			if (r.get() == resource.get())
			{
				__debugbreak();
				throw std::runtime_error("asd");
			}
		}
	}

	buckets[current_bucket].push_back(resource);
}

void ResourceReleaser::Swap()
{
	std::scoped_lock lock(mutex);
	current_bucket = (current_bucket + 1) % bucket_count;
	auto next_bucket = (current_bucket + 1) % bucket_count;
	ClearBucket(next_bucket);
}

void ResourceReleaser::ClearBucket(int index)
{
	for (int i = 0; i < buckets[index].size(); i++)
	{
		buckets[index][i].reset();
	}

	buckets[index].clear();
}

void ResourceReleaser::Clear()
{
	std::scoped_lock lock(mutex);

	for (current_bucket = 0; current_bucket < buckets.size(); current_bucket++)
	{
		ClearBucket(current_bucket);
	}

	current_bucket = 0;
}