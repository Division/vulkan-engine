#pragma once

#include <iostream>
#include "Profiler.h"
#include <vector>
#include <memory>
#include "utils/Math.h"

namespace Memory {

	template <typename T, Tag tag>
	class TaggedAllocator : public std::allocator<T>
	{
	public:
		T* allocate(size_t count)
		{
			auto size = sizeof(T) * count;
			T* result = (T*)_aligned_malloc(size, 16);
			Profiler::OnAllocation(size, tag);
			return result;
		}

		T* allocate_aligned(size_t count, size_t alignment)
		{
			auto size = sizeof(T) * count;
			T* result = (T*)_aligned_malloc(size, (size_t)NextPowerOfTwo((uint32_t)alignment));
			Profiler::OnAllocation(size, tag);
			return result;
		}

		void deallocate(T* ptr, size_t count)
		{
			_aligned_free(ptr);
			Profiler::OnDeallocation(count * sizeof(T), tag);
		}
	};

}