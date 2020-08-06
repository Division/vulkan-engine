#include "SceneBuffers.h"

namespace render {

	SceneBuffers::SceneBuffers()
	{
		camera_buffer = std::make_unique<Device::DynamicBuffer<Device::ShaderBufferStruct::Camera>>(256 * 30);
		object_params = std::make_unique<Device::DynamicBuffer<Device::ShaderBufferStruct::ObjectParams>>(sizeof(Device::ShaderBufferStruct::ObjectParams) * 3000);
		skinning_matrices = std::make_unique<Device::DynamicBuffer<Device::ShaderBufferStruct::SkinningMatrices>>(sizeof(Device::ShaderBufferStruct::SkinningMatrices) * 200);
	}

}