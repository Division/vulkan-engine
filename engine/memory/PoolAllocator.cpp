#include "PoolAllocator.h"

namespace core { namespace Memory {

	PoolAllocatorBase::PoolAllocatorBase(size_t element_size, size_t count, void* memory)
		: element_size(element_size)
		, pool_item_size(element_size)
		, element_count(count)
		, pool_memory(memory)
		, free_list_start(nullptr)
		, allocated_count(0)
	{
		if (element_size < sizeof(size_t))
			throw std::runtime_error("element size can't be smaller than pointer size");

		SetupFreeList();
	}

	PoolAllocatorBase::~PoolAllocatorBase() = default;

	void* PoolAllocatorBase::Allocate()
	{
		std::scoped_lock<std::mutex> lock(mutex);

		void** const item = (void**)free_list_start;

		if (item != nullptr)
		{
			allocated_count += 1;
			free_list_start = *item;
		}

		return item;
	}

	void PoolAllocatorBase::Deallocate(void* item)
	{
		std::scoped_lock<std::mutex> lock(mutex);

		void** const new_first_item = (void**)item;
		*new_first_item = free_list_start;
		free_list_start = new_first_item;
		allocated_count -= 1;
	}

	void PoolAllocatorBase::SetupFreeList()
	{
		free_list_start = pool_memory;
		for (size_t i = 0; i < element_count; i++)
		{
			void** pool_item_memory = (void**)((char*)pool_memory + pool_item_size * i);
			// Writing next pointer to the start of the item memory
			*pool_item_memory = i < element_count - 1 ? (char*)pool_memory + pool_item_size * (i + 1) : nullptr;
		}
	}

} }