#pragma once

#include <memory>

namespace core
{
	namespace Device
	{
		class Texture;
		class VulkanRenderState;
		class ShaderProgram;
		class ShaderCache;
	}
}

class Mesh;

namespace core { namespace render { namespace effects {

	class Skybox
	{
	public:
		Skybox(core::Device::ShaderCache& shader_cache);
		~Skybox();

		void SetTexture(core::Device::Texture* texture);
		void Render(core::Device::VulkanRenderState& state);

	private:
		core::Device::Texture* cubemap_texture = nullptr;
		std::unique_ptr<Mesh> cube_mesh;
		core::Device::ShaderProgram* shader;
	};

} } }