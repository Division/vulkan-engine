#pragma once

#include <string>
#include <array>
#include <atomic>

namespace Memory {

	enum Tag : int
	{
		Render = 0,
		JobSystem,
		Texture,
		ECS,
		Physics,
		UnknownResource,
		Unknown,
		Count
	};

	namespace Profiler
	{
		struct AllocationsData
		{
			std::atomic_uint64_t allocated_size = 0;
			std::atomic_uint64_t num_allocations = 0;
			std::atomic_uint32_t num_allocations_current_frame = 0;
		};

		struct AllocationsDataSnapshot
		{
			uint64_t allocated_size = 0;
			uint64_t num_allocations = 0;
			uint32_t num_allocations_current_frame = 0;
		};

		typedef std::array<AllocationsDataSnapshot, Tag::Count> Snapshot;

		const std::string& GetTagName(Tag tag);
		
		const AllocationsData& GetAllocationsData(Tag tag);

		void StartFrame();

		void OnAllocation(size_t size, Tag tag);
		void OnDeallocation(size_t size, Tag tag);

		size_t GetFrameAllocations();
		size_t GetFrameAllocationsSize();

		void MakeSnapshot();
		void ValidateSnapshot();
	};

}