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
		auto* GetSkinningMatricesBuffer() const { return skinning_matrices.get(); };

	private:
		std::unique_ptr<Device::ConstantBuffer> constant_buffer;
		std::unique_ptr<Device::ConstantBuffer> skinning_matrices;
	};

}