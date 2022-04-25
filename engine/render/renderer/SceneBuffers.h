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
		auto* GetDrawCallInstancesBuffer() const { return draw_call_instances.get(); };
		auto* GetMaterialUniformsBuffer() const { return material_uniforms.get(); }
		auto* GetUserUniformsBuffer() const { return user_uniforms.get(); }

	private:
		std::unique_ptr<Device::ConstantBuffer> constant_buffer;
		std::unique_ptr<Device::ConstantBuffer> skinning_matrices;
		std::unique_ptr<Device::ConstantBuffer> draw_call_instances;
		std::unique_ptr<Device::ConstantBuffer> material_uniforms;
		std::unique_ptr<Device::ConstantBuffer> user_uniforms;
	};

}