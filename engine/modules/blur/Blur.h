#pragma once

#include "render/renderer/SceneRenderer.h"

namespace Device
{
	class ShaderCache;
	class ShaderProgram;
	class ResourceBindings;
}

class Mesh;

namespace Modules
{
	struct RendererResources;

	class Blur
	{
	public:
		Blur(const char* shader_dir);

		// To be passed into rpsProgramBindNodeSubprogram.
		// Add this node to your rpsl:
		// graphics node Blur(rtv backBuffer : SV_Target0, float sigma, srv srcTexture);
		RpsSubprogram GetSubprogram() const { return *program; }

	private:
		void Render(const RpsCmdCallbackContext* pContext);
		render::RpsSubprogramHandle program;
		Mesh* full_screen_quad_mesh;
		Device::ShaderProgram* shader_blur;
	};

}