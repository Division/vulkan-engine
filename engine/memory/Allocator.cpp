#include "Allocator.h"

void* operator new(size_t size)
{
	auto result = malloc(size);
	Memory::Profiler::OnAllocation(size, Memory::Tag::Unknown);
	return result;
}

void* operator new[](size_t size)
{
	auto result = malloc(size);
	Memory::Profiler::OnAllocation(size, Memory::Tag::Unknown);
	return result;
}

void operator delete(void* ptr, std::size_t size)
{
	free(ptr);
	Memory::Profiler::OnDeallocation(size, Memory::Tag::Unknown);
}

void operator delete[](void* ptr, std::size_t size)
{
	free(ptr);
	Memory::Profiler::OnDeallocation(size, Memory::Tag::Unknown);
}