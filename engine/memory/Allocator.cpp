#include "Allocator.h"

void* operator new(size_t size)
{
	auto result = malloc(size);
	core::Memory::Profiler::OnAllocation(size, core::Memory::Tag::Unknown);
	return result;
}

void* operator new[](size_t size)
{
	auto result = malloc(size);
	core::Memory::Profiler::OnAllocation(size, core::Memory::Tag::Unknown);
	return result;
}

void operator delete(void* ptr, std::size_t size)
{
	free(ptr);
	core::Memory::Profiler::OnDeallocation(size, core::Memory::Tag::Unknown);
}

void operator delete[](void* ptr, std::size_t size)
{
	free(ptr);
	core::Memory::Profiler::OnDeallocation(size, core::Memory::Tag::Unknown);
}