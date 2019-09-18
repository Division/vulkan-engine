#pragma once

#include "CommonIncludes.h"
#include "render/mesh/Mesh.h"
#include "render/shader/ShaderBufferStruct.h"
#include "render/material/Material.h"

namespace core {namespace Device {

	struct RenderOperation 
	{
		std::shared_ptr<Mesh> mesh;
		std::shared_ptr<Material> material;
		ShaderBufferStruct::ObjectParams *object_params = nullptr;
		ShaderBufferStruct::SkinningMatrices *skinning_matrices = nullptr;

		//GLenum mode = GL_TRIANGLES;
		bool depth_test = true;
		//MultiBufferAddress objectParamsBlockOffset;
		//MultiBufferAddress skinningOffset;
		std::string *debug_info = nullptr;
	};

} }
