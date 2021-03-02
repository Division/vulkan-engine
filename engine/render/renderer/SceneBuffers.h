#pragma once

#include "CommonIncludes.h"
#include "render/buffer/DynamicBuffer.h"
#include "render/shader/ShaderBufferStruct.h"

namespace render {

	class SceneBuffers
	{
	public:
		SceneBuffers();

		auto* GetCameraBuffer() const { return camera_buffer.get(); };
		auto* GetEnvironmentSettingsBuffer() const { return environment_settings.get(); };
		auto* GetObjectParamsBuffer() const { return object_params.get(); };
		auto* GetSkinningMatricesBuffer() const { return skinning_matrices.get(); };

	private:
		std::unique_ptr<Device::DynamicBuffer<Device::ShaderBufferStruct::Camera>> camera_buffer;
		std::unique_ptr<Device::DynamicBuffer<Device::ShaderBufferStruct::ObjectParams>> object_params;
		std::unique_ptr<Device::DynamicBuffer<Device::ShaderBufferStruct::SkinningMatrices>> skinning_matrices;
		std::unique_ptr<Device::DynamicBuffer<Device::ShaderBufferStruct::EnvironmentSettings>> environment_settings;

	};

}