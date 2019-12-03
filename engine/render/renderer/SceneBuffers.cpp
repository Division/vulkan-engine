#include "SceneBuffers.h"

namespace core { namespace render {

	SceneBuffers::SceneBuffers()
	{
		camera_buffer = std::make_unique<UniformBuffer<ShaderBufferStruct::Camera>>(256 * 30);
		object_params = std::make_unique<UniformBuffer<ShaderBufferStruct::ObjectParams>>(sizeof(ShaderBufferStruct::ObjectParams) * 3000);
		skinning_matrices = std::make_unique<UniformBuffer<ShaderBufferStruct::SkinningMatrices>>(sizeof(ShaderBufferStruct::SkinningMatrices) * 200);
	}

} }