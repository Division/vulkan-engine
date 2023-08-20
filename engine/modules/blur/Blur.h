#pragma once

namespace Device
{
	class ShaderCache;
	class ShaderProgram;
	class ResourceBindings;
}

class Mesh;
struct RpsCmdCallbackContext;

namespace Modules
{
	struct RendererResources;

	class Blur
	{
	public:
		Blur(const char* shader_dir);

		void Render(const RpsCmdCallbackContext* pContext);

	private:
		Mesh* full_screen_quad_mesh;
		Device::ShaderProgram* shader_blur;
	};

}