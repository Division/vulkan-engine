#include "Device.h"
#include "VulkanContext.h"

namespace core { namespace Device {

	Device::Device(GLFWwindow* window)
	{
		context = std::make_unique<VulkanContext>(window);
	}

} }