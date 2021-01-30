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

size_t ResourceReleaser::ClearBucket(int index)
{
	const size_t cleared_count = buckets[index].size();

	for (int i = 0; i < buckets[index].size(); i++)
	{
		buckets[index][i].reset();
	}

	buckets[index].clear();

	return cleared_count;
}

size_t ResourceReleaser::Clear()
{
	std::scoped_lock lock(mutex);

	size_t cleared_count = 0;

	for (current_bucket = 0; current_bucket < buckets.size(); current_bucket++)
	{
		cleared_count += ClearBucket(current_bucket);
	}

	current_bucket = 0;
	return cleared_count;
}