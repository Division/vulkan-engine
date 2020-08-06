#include "Skybox.h"
#include "utils/MeshGeneration.h"
#include "render/shader/ShaderCache.h"
#include "render/device/VulkanRenderState.h"
#include "ecs/components/DrawCall.h"

using namespace Device;
using namespace ECS;

namespace render { namespace effects {

	Skybox::~Skybox() = default;
	
	Skybox::Skybox(Device::ShaderCache& shader_cache)
	{
		cube_mesh = std::make_unique<Mesh>(false);
		MeshGeneration::generateBox(cube_mesh.get(), 1, 1, 1);
		cube_mesh->createBuffer();

		auto shader_info = ShaderProgramInfo()
			.AddShader(ShaderProgram::Stage::Vertex, L"shaders/skybox.hlsl", "vs_main")
			.AddShader(ShaderProgram::Stage::Fragment, L"shaders/skybox.hlsl", "ps_main");

		shader = shader_cache.GetShaderProgram(shader_info);
	}

	void Skybox::SetTexture(Device::Texture* texture)
	{
		cubemap_texture = texture;
	}

	void Skybox::Render(Device::VulkanRenderState& state)
	{
		RenderMode mode;
		mode.SetDepthWriteEnabled(false);
		mode.SetDepthTestEnabled(false);
		mode.SetCullMode(CullMode::Front);

		state.SetRenderMode(mode);
		components::DrawCall draw_call;
		draw_call.shader = shader;
		draw_call.mesh = cube_mesh.get();
		draw_call.descriptor_set = nullptr;
		state.RenderDrawCall(&draw_call, false);
	}

} }