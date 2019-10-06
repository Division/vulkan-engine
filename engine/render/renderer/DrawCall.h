#pragma once

#include "CommonIncludes.h"

class Mesh;

namespace core
{
	namespace Device
	{
		class ShaderBindings;
		class ShaderProgram;
	}
}


namespace core { namespace render {

	using namespace core::Device;

	struct DrawCall
	{
		DrawCall();
		~DrawCall();

		const Mesh* mesh = nullptr;
		ShaderProgram* shader = nullptr;
		std::unique_ptr<ShaderBindings> shader_bindings;
	};

} }