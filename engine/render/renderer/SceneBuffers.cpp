#include "SceneBuffers.h"

namespace core { namespace render {

	SceneBuffers::SceneBuffers()
	{
		camera_buffer = std::make_unique<UniformBuffer<ShaderBufferStruct::Camera>>(sizeof(ShaderBufferStruct::Camera));
		object_params = std::make_unique<UniformBuffer<ShaderBufferStruct::ObjectParams>>(sizeof(ShaderBufferStruct::ObjectParams) * 3000);
	}

} }