#pragma once

#include <vector>
#include <deque>
#include "Allocator.h"

namespace core { namespace Memory {

	template <typename T, Tag tag>
	using Vector = std::vector<T, TaggedAllocator<T, tag>>;

	//template <typename T, Tag tag>
	//using Deque = std::deque<T, TaggedAllocator<T, tag>>;

	template <typename T, Tag tag>
	class Pointer
	{
	public:
		Pointer()
		: pointer(nullptr, deleter) {}

		template<typename... Args>
		static Pointer<T, tag> Create(Args&&... args)
		{
			TaggedAllocator<T, tag> allocator;
			sizeof(allocator);
			T* instance = allocator.allocate(1);
			new (instance) T(std::forward<Args>(args)...);
			return Pointer<T, tag>(std::unique_ptr<T, void(*)(T*)>(instance, deleter));
		}

		static void deleter(T* ptr)
		{
			if (ptr)
			{
				ptr->~T();
				TaggedAllocator<T, tag> allocator;
				allocator.deallocate(ptr, 1);
			}
		}

		auto get() { return pointer.get(); }
		const auto get() const { return pointer.get(); }

		auto operator=(const Pointer<T, tag>& other) = delete;
		auto& operator=(Pointer<T, tag>&& other)
		{
			pointer = std::move(other.pointer);
			return *this;
		}

		auto operator->() { return pointer.get(); }
		auto operator*() { return *pointer; }
		operator bool() { return (bool)pointer; }

	private:
		Pointer(std::unique_ptr<T, void(*)(T*)> ptr)
			: pointer(std::move(ptr))
		{}

		std::unique_ptr<T, void(*)(T*)> pointer;
	};

}}