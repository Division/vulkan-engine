#pragma once

#include <stdint.h>
#include <array>

namespace Device
{
	class VulkanCommandBuffer;
}

namespace render::profiler
{
	enum class ProfilerName : uint32_t
	{
		PassUnknown,
		PassMain,
		PassDepthPrepass,
		PassShadowmap,
		PassCompute,
		PassPostProcess,
		PassDebugUI,
		PassCount,
		RecordPasses = PassCount,
		Count,
	};

	typedef std::array<double, (uint32_t)ProfilerName::Count> ProfilerTimings;

	struct ProfilerQuery
	{
		ProfilerName id = ProfilerName::PassUnknown;
		uint32_t pass_index;
		ProfilerQuery(ProfilerName id, uint32_t pass_index) : id(id), pass_index(pass_index) {}
	};

	const std::string& GetProfilerDisplayName(ProfilerName name);
	void Initialize();
	void Deinitialize();
	void Update();
	void StartMeasurement(Device::VulkanCommandBuffer& command_buffer, uint32_t pass_index, ProfilerName id);
	void FinishMeasurement(Device::VulkanCommandBuffer& command_buffer, uint32_t pass_index, ProfilerName id);
	void GetProfilerTimings(ProfilerTimings& out_timings);
}
