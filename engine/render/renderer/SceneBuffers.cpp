#include "SceneBuffers.h"

namespace render {

	SceneBuffers::SceneBuffers()
	{
		constant_buffer = std::make_unique<Device::ConstantBuffer>("Main CB", 1 * 1024 * 1024); // 4mb
		skinning_matrices = std::make_unique<Device::ConstantBuffer>("SkinningMatrices", 4 * 1024 * 1024, Device::BufferType::Storage); // 4mb
		draw_call_instances = std::make_unique<Device::ConstantBuffer>("DrawCallInstances", 1 * 1024 * 1024, Device::BufferType::Storage); // 1mb
		material_uniforms = std::make_unique<Device::ConstantBuffer>("MaterialUniforms", 2 * 1024 * 1024, Device::BufferType::Storage); // 2mb
		user_uniforms = std::make_unique<Device::ConstantBuffer>("UserUniforms", 2 * 1024 * 1024, Device::BufferType::Storage); // 2mb
	}

}