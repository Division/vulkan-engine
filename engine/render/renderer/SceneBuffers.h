#pragma once

#include "CommonIncludes.h"
#include "render/buffer/DynamicBuffer.h"
#include "render/shader/ShaderBufferStruct.h"

namespace core { namespace render {

	using namespace Device;

	class SceneBuffers
	{
	public:
		SceneBuffers();

		auto* GetCameraBuffer() const { return camera_buffer.get(); };
		auto* GetObjectParamsBuffer() const { return object_params.get(); };
		auto* GetSkinningMatricesBuffer() const { return skinning_matrices.get(); };

	private:
		std::unique_ptr<DynamicBuffer<ShaderBufferStruct::Camera>> camera_buffer;
		std::unique_ptr<DynamicBuffer<ShaderBufferStruct::ObjectParams>> object_params;
		std::unique_ptr<DynamicBuffer<ShaderBufferStruct::SkinningMatrices>> skinning_matrices;

	};

} }