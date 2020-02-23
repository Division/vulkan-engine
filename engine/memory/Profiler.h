#pragma once

#include <string>
#include <array>

namespace core { namespace Memory {

	enum Tag : int
	{
		Render = 0,
		JobSystem,
		Texture,
		ECS,
		Unknown,
		Count
	};

	namespace Profiler
	{
		struct AllocationsData
		{
			uint64_t allocated_size;
			uint64_t num_allocations;
			uint32_t num_allocations_current_frame;
		};

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

} }