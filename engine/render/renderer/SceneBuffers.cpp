#include "SceneBuffers.h"

namespace render {

	SceneBuffers::SceneBuffers()
	{
		constant_buffer = std::make_unique<Device::ConstantBuffer>(4 * 1024 * 1024); // 4mb
		object_params = std::make_unique<Device::DynamicBuffer<Device::ShaderBufferStruct::ObjectParams>>(sizeof(Device::ShaderBufferStruct::ObjectParams) * 3000);
		skinning_matrices = std::make_unique<Device::DynamicBuffer<Device::ShaderBufferStruct::SkinningMatrices>>(sizeof(Device::ShaderBufferStruct::SkinningMatrices) * 200);
	}

}