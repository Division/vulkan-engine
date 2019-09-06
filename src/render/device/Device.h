#pragma once

#include "CommonIncludes.h"

namespace core { namespace Device {

	class VulkanContext;

	// TODO: remove Device
	class Device
	{
	public:
		Device(GLFWwindow* window);
		virtual ~Device() = default;

		VulkanContext* GetContext() const { return context.get(); }

	private:
		std::unique_ptr<VulkanContext> context;
	};

} }