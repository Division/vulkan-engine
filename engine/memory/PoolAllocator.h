#pragma once

#include "Allocator.h"
#include <mutex>

namespace Memory {

	class PoolAllocatorBase
	{
	public:
		PoolAllocatorBase(size_t element_size, size_t count, void* memory);
		virtual ~PoolAllocatorBase();

		void* Allocate();
		void Deallocate(void* item);
		size_t GetElementSize() const { return element_size; }
	
		bool CanAllocate() const { return allocated_count < element_count; }
		size_t GetAllocatedCount() const { return allocated_count; }
		const void* GetFreeListStart() const { return free_list_start; }

	protected:
		void SetupFreeList();

	protected:
		size_t allocated_count;
		size_t pool_item_size; // may be different from element size if alignment added
		size_t element_size; 
		size_t element_count; 
		void* pool_memory;
		void* free_list_start;
		std::mutex mutex;
	};

	template <Tag tag>
	class PoolAllocator : public PoolAllocatorBase
	{
	public:
		PoolAllocator(size_t element_size, size_t count)
			: PoolAllocatorBase(element_size, count, allocator.allocate(GetPoolMemorySize(element_size, count))) 
		{}

		~PoolAllocator()
		{
			allocator.deallocate((char*)pool_memory, GetPoolMemorySize(element_size, element_count));
		}
	
	private:
		size_t GetPoolMemorySize(size_t element_size, size_t count)
		{
			return element_size * count;
		}

	private:
		TaggedAllocator<char, tag> allocator;

	};

}