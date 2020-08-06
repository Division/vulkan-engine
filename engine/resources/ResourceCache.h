#pragma once

#include <atomic>
#include <unordered_map>
#include <memory>
#include "memory/Allocator.h"
#include <fstream>

namespace Resources
{
	class ResourceBase
	{
	public:
		template<typename> friend class Handle;

		ResourceBase(const std::wstring& path) 
			: filename(path)
			, resource_memory(nullptr)
			, reference_counter(0)
			, loaded(false)
		{};

		virtual ~ResourceBase() {}

		uint32_t GetRefCount() const { return reference_counter; }
		bool Loaded() { return loaded; }
		void Resolve() {};

	protected:
		
		void AddRef()
		{
			reference_counter += 1;
		}

		void RemoveRef()
		{
			reference_counter -= 1;
		}

		std::ifstream GetStream()
		{
			std::ifstream stream(filename, std::ios::binary);
			return stream;
		}

		std::wstring filename;
		std::atomic_bool loaded;
		std::atomic_uint32_t reference_counter;
		void* resource_memory;
	};

	template <typename T, Memory::Tag Tag = Memory::Tag::UnknownResource>
	class Resource : public ResourceBase
	{
	private:
		using Allocator = Memory::TaggedAllocator<T, Tag>;

	public:

		Resource(const std::wstring& path) : ResourceBase(path)
		{
			Allocator allocator;
			resource_memory = allocator.allocate(1);
			auto stream = GetStream();
			new (resource_memory) T(stream);
		}

		~Resource()
		{
			reinterpret_cast<T*>(resource_memory)->~T();

			Allocator allocator;
			allocator.deallocate(reinterpret_cast<T*>(resource_memory), 1);
			resource_memory = nullptr;
		}

		T& operator*()
		{
			Resolve();
			return *reinterpret_cast<T*>(resource_memory);
		}

		T* operator->()
		{
			Resolve();
			return reinterpret_cast<T*>(resource_memory);
		}
	};

	template <typename T>
	class Handle
	{
	public:
		Handle() : resource(nullptr) {}

		explicit Handle(const std::wstring& filename)
		{
			resource = static_cast<Resource<T>*>(Cache::Get().GetResource(filename));
			if (!resource)
			{
				auto unique_resource = std::make_unique<Resource<T>>(filename);
				resource = unique_resource.get();
				Cache::Get().SetResource(filename, std::move(unique_resource));
			}

			resource->AddRef();
		}

		~Handle()
		{
			if (resource)
				resource->RemoveRef();
		}

		T& operator*()
		{
			return resource->operator*();
		}

		T* operator->()
		{
			return resource->operator->();
		}

	private:
		Resource<T>* resource;
	};

	class Cache
	{
	public:
		template<typename> friend class Handle;
		static Cache& Get();

		void GCCollect();
		void Destroy();

	private:
		ResourceBase* GetResource(const std::wstring& filename);
		void SetResource(const std::wstring& filename, std::unique_ptr<ResourceBase> resource);
		std::unordered_map<std::wstring, std::unique_ptr<ResourceBase>> resources;
	};
}