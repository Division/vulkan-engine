#pragma once

#include <atomic>
#include <unordered_map>
#include <memory>
#include <sstream>
#include "memory/Allocator.h"
#include <fstream>
#include "system/JobSystem.h"
#include <optick/src/optick.h>
#include <mutex>
#include "utils/NonCopyable.h"

namespace Resources
{
	class ResourceBase
	{
	public:
		template<typename> friend class Handle;
		friend class Cache;

		enum class State : int
		{
			Loading,
			Loaded,
			Unloading,
			Unloaded,
			Failed
		};

		typedef std::unique_ptr<void, std::function<void(void*)>> ParamPtr;
		typedef void (*CreateCallback)(void*, void*);

		explicit ResourceBase(const wchar_t* path, uint32_t hash, ParamPtr param, CreateCallback create_callback)
			: filename(path)
			, hash(hash)
			, resource_memory(nullptr)
			, reference_counter(0)
			, state(State::Unloaded)
			, create_callback(create_callback)
			, param(std::move(param))
		{};

		virtual ~ResourceBase() 
		{
		}

		uint32_t GetRefCount() const { return reference_counter; }
		uint32_t GetHash() const { return hash; }
		bool IsResolved() { return state == State::Loaded; }
		const std::wstring& GetFilename() const { return filename; }

	protected:
		struct WCharInitializer
		{
			explicit WCharInitializer(const wchar_t* str) : value(str) {};
			uint32_t GetHash() const { return FastHash(value); }
			const wchar_t* GetPath() const { return value; }

		private:
			const wchar_t* value;
		};

		bool TransitionState(State old_state, State new_state)
		{
			return state.compare_exchange_strong(old_state, new_state);
		}

		void AddRef()
		{
			reference_counter += 1;
		}

		void RemoveRef()
		{
			reference_counter -= 1;
		}

		std::atomic<State> state;
		std::atomic_uint32_t reference_counter;
		std::wstring filename;
		uint32_t hash;
		void* resource_memory;
		ParamPtr param;

		CreateCallback create_callback;
	};

	template <typename T, Memory::Tag Tag = Memory::Tag::UnknownResource>
	class Resource : public ResourceBase
	{
	private:
		using Allocator = Memory::TaggedAllocator<T, Tag>;

		class JobLoadResource : public Thread::Job
		{
		public:
			JobLoadResource(Resource* resource)
				: resource(resource)
			{}

			virtual void Execute() override
			{
				try
				{
					Allocator allocator;
					resource->resource_memory = allocator.allocate(1);
					resource->create_callback(resource->resource_memory, resource->param.get());
					resource->state = State::Loaded;
				}
				catch (...)
				{
					resource->state = State::Failed;
					throw;
				}
			}

		private:
			Resource* resource;
		};

	private:
		template<typename T> friend class Handle;
		friend class Cache;

		void Load()
		{
			if (!TransitionState(State::Unloaded, State::Loading))
				return;

			Thread::Scheduler::Get().SpawnJob<JobLoadResource>(Thread::Job::Priority::Low, this);
		};

		void Unload()
		{
			if (!TransitionState(State::Loaded, State::Unloading))
				return;

			reinterpret_cast<T*>(resource_memory)->~T();

			Allocator allocator;
			allocator.deallocate(reinterpret_cast<T*>(resource_memory), 1);
			resource_memory = nullptr;

			state = State::Unloaded;
		};

		void Wait(State target_state)
		{
			while ((int)state.load() < (int)target_state)
				std::this_thread::yield();
		}

		void WaitUnload()
		{
			if (state == State::Loading)
				Wait(State::Loaded);

			if (state == State::Loaded)
				Unload();

			if (state == State::Unloading)
				Wait(State::Unloaded);
		}


	public:
		Resource(const ResourceBase::WCharInitializer& initializer) : ResourceBase(initializer.GetPath(), initializer.GetHash()
			, ResourceBase::ParamPtr(new std::wstring(initializer.GetPath()), [](void* ptr) { delete reinterpret_cast<std::wstring*>(ptr); })
			, [](void* resource, void* param) { new (resource) T(*reinterpret_cast<std::wstring*>(param)); }
		) {}

		template<typename I>
		Resource(const I& initializer) : ResourceBase(initializer.GetPath(), initializer.GetHash()
			, ResourceBase::ParamPtr(new I(initializer), [](void* ptr) { delete reinterpret_cast<I*>(ptr); })
			, [](void* resource, void* param) { new (resource) T(*reinterpret_cast<I*>(param)); }
		) {}

		~Resource()
		{
			WaitUnload();
		}

		void Resolve()
		{
			if (IsResolved()) return;

			OPTICK_EVENT();

			if (state == State::Unloading)
				Wait(State::Unloaded);

			if (state == State::Unloaded)
				Load();

			if (state == State::Loading)
				Wait(State::Loaded);

			if (state == State::Failed)
				throw MessageException(filename);
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
		Handle(nullptr_t) : resource(nullptr) {}

		template<typename HI>
		Handle(const HI& initializer)
		{
			resource = static_cast<Resource<T>*>(Cache::Get().GetOrCreateResource<T>(initializer));
		}

		explicit Handle(const std::wstring& filename) : Handle(ResourceBase::WCharInitializer(filename.c_str())) {}
		explicit Handle(const wchar_t* filename) : Handle(ResourceBase::WCharInitializer(filename)) {}

		Handle(const Handle& other)
		{
			resource = other.resource;
			if (resource)
				resource->AddRef();
		}

		Handle(Handle&& other)
		{
			resource = other.resource;
			other.resource = nullptr;
		}

		Handle& operator=(nullptr_t)
		{
			if (resource)
				resource->RemoveRef();

			resource = nullptr;

			return *this;
		}
		
		Handle& operator=(const Handle& other)
		{
			if (resource)
				resource->RemoveRef();

			resource = other.resource;
			if (resource)
				resource->AddRef();

			return *this;
		}

		Handle& operator=(Handle&& other)
		{
			if (resource)
				resource->RemoveRef();

			resource = other.resource;
			other.resource = nullptr;
			return *this;
		}

		~Handle()
		{
			if (resource)
				resource->RemoveRef();
		}

		const T& operator*() const
		{
			return resource->operator*();
		}

		const T* operator->() const
		{
			return resource->operator->();
		}

		operator bool() const { return (bool)resource; }

		bool IsResolved() const { return resource && resource->IsResolved(); }
		const std::wstring GetFilename() const { return resource ? resource->GetFilename() : L""; }

	private:
		Resource<T>* resource;
	};

	class Cache : public NonCopyable
	{
	public:
		template<typename> friend class Handle;
		static Cache& Get();

		size_t GCCollect();
		void Destroy();

	private:
		template<typename T, typename I>
		ResourceBase* GetOrCreateResource(I initializer)
		{
			std::scoped_lock<std::recursive_mutex> lock(mutex);

			auto* resource = static_cast<Resource<T>*>(GetResource(initializer.GetHash()));
			if (!resource)
			{
				auto unique_resource = std::make_unique<Resource<T>>(initializer);
				resource = unique_resource.get();
				SetResource(resource->GetHash(), std::move(unique_resource));
			}

			resource->AddRef();
			if (!resource->IsResolved())
				resource->Load();

			return resource;
		}

		ResourceBase* GetResource(uint32_t key);
		void SetResource(uint32_t key, std::unique_ptr<ResourceBase> resource);
	
	private:
		std::recursive_mutex mutex;
		std::unordered_map<uint32_t, std::unique_ptr<ResourceBase>> resources;
	};

	class Exception : public std::exception
	{
	public:
		Exception() {}
		Exception(const std::wstring& filename);
		Exception(const Exception& other);
		virtual ~Exception() = default;

		template<typename T> 
		Exception& operator<<(const T& item) { message_stream << item; return *this; }

		virtual const char* what() const override { error = message_stream.str(); return error.c_str(); }

	private:
		mutable std::string error;
		std::stringstream message_stream;
	};

	class MessageException : public Exception
	{
	public:
		MessageException() : Exception() {}
		MessageException(const std::wstring& filename);
		MessageException(const MessageException& other);
		virtual ~MessageException() = default;

	private:
		mutable std::string error;
		std::stringstream message_stream;
	};
}