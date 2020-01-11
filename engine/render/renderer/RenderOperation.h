#pragma once

#include "CommonIncludes.h"
#include "render/mesh/Mesh.h"
#include "render/shader/ShaderBufferStruct.h"
#include "render/material/Material.h"

namespace core { namespace Device {

	class ShaderProgram;

	struct RenderOperation 
	{
		const Mesh* mesh;
		Material* material;
		//ShaderProgram* shader;
		//vk::Buffer object_params_buffer;
		ShaderBufferStruct::ObjectParams *object_params = nullptr;
		ShaderBufferStruct::SkinningMatrices *skinning_matrices = nullptr;

		//size_t object_params_buffer_offset = 0;
		//size_t skinning_matrices_buffer_offset = 0;
		//MultiBufferAddress skinningOffset;
		std::string *debug_info = nullptr;
	};

} }
