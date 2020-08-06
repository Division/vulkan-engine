#pragma once

#include <mutex>
#include <atomic>
#include <array>
#include <vector>

namespace Device
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
		static constexpr uint32_t bucket_count = 4;
		static ResourceReleaser& Get();

		void Add(std::shared_ptr<Resource> resource);
		void Swap();
		void Clear();

	private:
		std::mutex mutex;
		uint32_t current_bucket = 0;
		std::array<std::vector<std::shared_ptr<Resource>>, bucket_count> buckets;
	};

	template <typename T>
	class Handle
	{
		static_assert(std::is_base_of<Resource, T>::value, "T must derive from Device::Resource");
	
	public:
		void AddToReleaser(std::shared_ptr<T>& res)
		{
			if (res && res.use_count() == 1)
				ResourceReleaser::Get().Add(std::static_pointer_cast<Resource>(res));
		}

		Handle() = default;

		Handle(std::unique_ptr<T> resource_src)
		{
			resource = std::move(resource_src);
		}

		Handle(const Handle& other)
		{
			resource = other.resource;
		}

		Handle(Handle&& other)
		{
			resource = std::move(other.resource);
		}

		Handle& operator=(const Handle& other)
		{
			AddToReleaser(resource);
			resource = other.resource;
			return *this;
		}

		Handle& operator=(Handle&& other)
		{
			AddToReleaser(resource);
			resource = std::move(other.resource);
			return *this;
		}

		bool operator==(const Handle& other)
		{
			return resource == other.resource;
		}

		~Handle()
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
		

	private:
		std::shared_ptr<T> resource;
	};
}