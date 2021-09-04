#include "SceneBuffers.h"

namespace render {

	SceneBuffers::SceneBuffers()
	{
		constant_buffer = std::make_unique<Device::ConstantBuffer>(4 * 1024 * 1024); // 4mb
		skinning_matrices = std::make_unique<Device::ConstantBuffer>(4 * 1024 * 1024); // 4mb
	}

}