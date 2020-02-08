#pragma once

#include <iostream>
#include "Profiler.h"
#include <vector>
#include <memory>

namespace core { namespace Memory {

	template <typename T, Tag tag>
	class TaggedAllocator : std::allocator<T>
	{
	public:
		T* allocate(size_t count)
		{
			auto size = sizeof(T) * count;
			T* result = (T*)malloc(size);
			Profiler::OnAllocation(size, tag);
			return result;
		}

		void deallocate(T* ptr, size_t count)
		{
			free(ptr);
			Profiler::OnDeallocation(count * sizeof(T), tag);
		}
	};

} }
