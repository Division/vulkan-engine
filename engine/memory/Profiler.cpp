#include "Profiler.h"
#include <mutex>

namespace core { namespace Memory {

	namespace Profiler
	{

		std::array<std::string, Tag::Count> tag_names = { "Render", "Texture", "ECS", "Unknown" };

		struct ProfilerData
		{
			std::array<AllocationsData, Tag::Count> allocations;
			std::mutex mutex;
			size_t total_frame_allocations;
			size_t total_frame_allocations_size;

			ProfilerData()
			{
				for (auto& alloc : allocations)
					alloc = { 0, 0, 0 };

				total_frame_allocations = 0;
				total_frame_allocations_size = 0;
			}
		};

		inline ProfilerData& GetProfilerData()
		{
			static ProfilerData data;
			return data;
		}

		size_t GetFrameAllocations()
		{
			return GetProfilerData().total_frame_allocations;
		}

		size_t GetFrameAllocationsSize()
		{
			return GetProfilerData().total_frame_allocations_size;
		}

		const std::string& GetTagName(Tag tag)
		{
			return tag_names[tag];
		}

		const AllocationsData& GetAllocationsData(Tag tag)
		{
			return GetProfilerData().allocations[tag];
		}

		void Initialize()
		{
		}

		void StartFrame()
		{
			for (auto& alloc : GetProfilerData().allocations)
				alloc.num_allocations_current_frame = 0;

			GetProfilerData().total_frame_allocations = 0;
			GetProfilerData().total_frame_allocations_size = 0;
		}

		void EndFrame()
		{

		}

		void OnAllocation(size_t size, Tag tag)
		{
			std::scoped_lock<std::mutex> lock(GetProfilerData().mutex);
			GetProfilerData().allocations[tag].allocated_size += size;
			GetProfilerData().allocations[tag].num_allocations += 1;
			GetProfilerData().allocations[tag].num_allocations_current_frame += 1;
			GetProfilerData().total_frame_allocations += 1;
			GetProfilerData().total_frame_allocations_size += size;
		}

		void OnDeallocation(size_t size, Tag tag)
		{
			std::scoped_lock<std::mutex> lock(GetProfilerData().mutex);
			GetProfilerData().allocations[tag].allocated_size -= size;
			GetProfilerData().allocations[tag].num_allocations -= 1;
		}

	};

} }