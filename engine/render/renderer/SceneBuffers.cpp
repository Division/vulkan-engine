#include "SceneBuffers.h"

namespace core { namespace render {

	SceneBuffers::SceneBuffers()
	{
		camera_buffer = std::make_unique<DynamicBuffer<ShaderBufferStruct::Camera>>(256 * 30);
		object_params = std::make_unique<DynamicBuffer<ShaderBufferStruct::ObjectParams>>(sizeof(ShaderBufferStruct::ObjectParams) * 3000);
		skinning_matrices = std::make_unique<DynamicBuffer<ShaderBufferStruct::SkinningMatrices>>(sizeof(ShaderBufferStruct::SkinningMatrices) * 200);
	}

} }