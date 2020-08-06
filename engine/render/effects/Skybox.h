#pragma once

#include <memory>

namespace Device
{
	class Texture;
	class VulkanRenderState;
	class ShaderProgram;
	class ShaderCache;
}

class Mesh;

namespace render { namespace effects {

	class Skybox
	{
	public:
		Skybox(Device::ShaderCache& shader_cache);
		~Skybox();

		void SetTexture(Device::Texture* texture);
		void Render(Device::VulkanRenderState& state);

	private:
		Device::Texture* cubemap_texture = nullptr;
		std::unique_ptr<Mesh> cube_mesh;
		Device::ShaderProgram* shader;
	};

} }