#include "render/shader/ShaderBindings.h"
#include "DrawCall.h"

namespace core { namespace render {

	DrawCall::DrawCall()
	{
		shader_bindings = std::make_unique<ShaderBindings>();
	};

	DrawCall::~DrawCall() = default;

} }