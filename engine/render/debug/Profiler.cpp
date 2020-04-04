#include "Profiler.h"
#include "CommonIncludes.h"
#include "Engine.h"
#include "render/device/VkObjects.h"
#include "render/device/VulkanContext.h"
#include <mutex>

namespace core::render::profiler
{
	struct FrameData
	{
		std::array<vk::UniqueQueryPool, 64> query_pools;
		std::vector<ProfilerQuery> queries;
		std::mutex mutex;
	};

	class Profiler
	{
	public:
		static const int ProfilerFrameCount = 3;

		Profiler()
		{
			auto device = Engine::Get()->GetVulkanDevice();

			vk::QueryPoolCreateInfo pool_info({}, vk::QueryType::eTimestamp, 2);

			for (auto& frame : frames)
				for (auto& pool : frame.query_pools)
					pool = device.createQueryPoolUnique(pool_info);
		}

		void StartMeasurement(core::Device::VulkanCommandBuffer& command_buffer, uint32_t pass_index, ProfilerName id)
		{
			ProfilerQuery query(id, pass_index);
			auto& frame = frames[current_frame];

			auto query_pool = frame.query_pools[pass_index].get();

			{
				std::scoped_lock<std::mutex> lock(frame.mutex);
				frame.queries.push_back(query);
			}

			auto device = Engine::Get()->GetVulkanDevice();
			command_buffer.GetCommandBuffer().resetQueryPool(query_pool, 0, 2);
			command_buffer.GetCommandBuffer().writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, query_pool, 0);
		}

		void FinishMeasurement(core::Device::VulkanCommandBuffer& command_buffer, uint32_t pass_index, ProfilerName id)
		{
			auto& frame = frames[current_frame];
			auto query_pool = frame.query_pools[pass_index].get();
			command_buffer.GetCommandBuffer().writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, query_pool, 1);
		}

		void GetProfilerTimings(ProfilerTimings& out_timings)
		{
			memset(out_timings.data(), 0, sizeof(out_timings));

			const double time_scale = Engine::Get()->GetContext()->GetDeviceProps().limits.timestampPeriod;
			auto frame_index = (current_frame + ProfilerFrameCount - 1) % ProfilerFrameCount;
			auto& frame = frames[frame_index];
			for (auto& query : frame.queries)
			{
				auto query_pool = frame.query_pools[query.pass_index].get();
				std::array<std::pair<uint64_t, uint64_t>, 2> timestamps = { std::make_pair((uint64_t)0, (uint64_t)0), std::make_pair((uint64_t)0, (uint64_t)0) };
				Engine::Get()->GetVulkanDevice().getQueryPoolResults(query_pool, 0, 2, sizeof(timestamps), timestamps.data(), sizeof(std::uint64_t) * 2, vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWithAvailability);
				
				if (timestamps[0].second && timestamps[1].second) // checking availability
				{
					auto dt = (timestamps[1].first - timestamps[0].first) * time_scale * 0.000'000'001;
					out_timings[(uint32_t)query.id] += dt;
				}
			}
		}

		void Update()
		{
			current_frame = (current_frame + 1) % ProfilerFrameCount;
			frames[current_frame].queries.clear();
		}

	private:
		uint32_t current_frame = 0;
		std::array<FrameData, ProfilerFrameCount> frames;
	};

	namespace
	{
		std::unique_ptr<Profiler> profiler;
		std::unique_ptr<std::array<std::string, (size_t)ProfilerName::Count>> profiler_names; // unique pointer to delete before memory profiler snapshot on exit
	}

	const std::string& GetProfilerDisplayName(ProfilerName name)
	{
		return (*profiler_names)[(size_t)name];
	}

	void Initialize()
	{
		profiler = std::make_unique<Profiler>();
		profiler_names = std::make_unique<std::array<std::string, (size_t)ProfilerName::Count>>();
		(*profiler_names)[(size_t)ProfilerName::PassUnknown] = "Unknown";
		(*profiler_names)[(size_t)ProfilerName::PassMain] = "Main";
		(*profiler_names)[(size_t)ProfilerName::PassDepthPrepass] = "DepthPrepass";
		(*profiler_names)[(size_t)ProfilerName::PassShadowmap] = "PassShadowmap";
		(*profiler_names)[(size_t)ProfilerName::PassCompute] = "PassCompute";
		(*profiler_names)[(size_t)ProfilerName::PassPostProcess] = "PassPostProcess";
		(*profiler_names)[(size_t)ProfilerName::PassDebugUI] = "PassDebugUI";
		(*profiler_names)[(size_t)ProfilerName::RecordPasses] = "Record command buffers";
	}

	void Deinitialize()
	{
		profiler = nullptr;
		profiler_names = nullptr;
	}

	void Update()
	{
		if (!profiler)
			return;

		profiler->Update();
	}

	void StartMeasurement(core::Device::VulkanCommandBuffer& command_buffer, uint32_t pass_index, ProfilerName id)
	{
		profiler->StartMeasurement(command_buffer, pass_index, id);
	}

	void FinishMeasurement(core::Device::VulkanCommandBuffer& command_buffer, uint32_t pass_index, ProfilerName id)
	{
		profiler->FinishMeasurement(command_buffer, pass_index, id);
	}

	void GetProfilerTimings(ProfilerTimings& out_timings)
	{
		profiler->GetProfilerTimings(out_timings);
	}
}