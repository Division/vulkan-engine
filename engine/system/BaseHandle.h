#pragma once

namespace System
{
	class Resource
	{
	public:
		virtual ~Resource() = default;
	private:
	};

	template <uint32_t BucketCount>
	class ResourceReleaser
	{
	public:
		void Add(std::shared_ptr<Resource> resource)
		{
			std::scoped_lock lock(mutex);

#if defined(_DEBUG)
			for (auto& list : buckets)
			{
				for (auto& r : list)
				{
					if (r.get() == resource.get())
					{
						throw std::runtime_error("resource double released");
					}
				}
			}
#endif

			buckets[current_bucket].push_back(resource);
		}

		void Swap()
		{
			std::scoped_lock lock(mutex);
			current_bucket = (current_bucket + 1) % BucketCount;
			ClearBucket(current_bucket);
		}

		size_t Clear()
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

		static ResourceReleaser& GetReleaser()
		{
			static ResourceReleaser releaser;
			return releaser;
		}

	private:
		size_t ClearBucket(int index)
		{
			const size_t cleared_count = buckets[index].size();

			for (int i = 0; i < buckets[index].size(); i++)
			{
				buckets[index][i].reset();
			}

			buckets[index].clear();

			return cleared_count;
		}

		std::recursive_mutex mutex;
		uint32_t current_bucket = 0;
		std::array<std::vector<std::shared_ptr<Resource>>, BucketCount> buckets;
	};

	template <typename T, typename Releaser>
	class BaseHandle
	{
	public:

		void AddToReleaser(std::shared_ptr<T>& res)
		{
			if (res && res.use_count() == 1)
				Releaser::GetReleaser().Add(std::static_pointer_cast<Resource>(res));
		}

		BaseHandle() = default;

		BaseHandle(std::nullptr_t) {}

		BaseHandle(std::unique_ptr<T> resource_src)
		{
			resource = std::move(resource_src);
		}

		BaseHandle(const BaseHandle& other)
		{
			resource = other.resource;
		}

		BaseHandle(BaseHandle&& other)
		{
			resource = std::move(other.resource);
		}

		BaseHandle& operator=(const BaseHandle& other)
		{
			AddToReleaser(resource);
			resource = other.resource;
			return *this;
		}

		BaseHandle& operator=(std::nullptr_t)
		{
			AddToReleaser(resource);
			resource = nullptr;
			return *this;
		}

		BaseHandle& operator=(BaseHandle&& other)
		{
			AddToReleaser(resource);
			resource = std::move(other.resource);
			return *this;
		}

		bool operator==(const BaseHandle& other)
		{
			return resource == other.resource;
		}

		bool operator!=(const BaseHandle& other)
		{
			return !(*this == other);
		}

		~BaseHandle()
		{
			AddToReleaser(resource);
		}

		void Reset()
		{
			resource.reset();
		}

		operator bool() const { return (bool)resource; }

		T* get() const { return resource.get(); }
		T& operator*() { return *resource; }
		T* operator->() { return resource.get(); }
		const T& operator*() const { return *resource; }
		const T* operator->() const { return resource.get(); }


	private:
		mutable std::shared_ptr<T> resource;
	};

}
