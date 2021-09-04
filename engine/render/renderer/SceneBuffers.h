#pragma once

#include "CommonIncludes.h"
#include "render/buffer/DynamicBuffer.h"
#include "render/buffer/ConstantBuffer.h"
#include "render/shader/ShaderBufferStruct.h"

namespace render {

	class SceneBuffers
	{
	public:
		SceneBuffers();

		auto* GetConstantBuffer() const { return constant_buffer.get(); }
		auto* GetObjectParamsBuffer() const { return object_params.get(); };
		auto* GetSkinningMatricesBuffer() const { return skinning_matrices.get(); };

	private:
		std::unique_ptr<Device::ConstantBuffer> constant_buffer;
		std::unique_ptr<Device::DynamicBuffer<Device::ShaderBufferStruct::ObjectParams>> object_params;
		std::unique_ptr<Device::DynamicBuffer<Device::ShaderBufferStruct::SkinningMatrices>> skinning_matrices;
	};

}