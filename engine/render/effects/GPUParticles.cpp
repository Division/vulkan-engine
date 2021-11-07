#include "GPUParticles.h"
#include "utils/MeshGeneration.h"
#include "render/shader/ShaderCache.h"
#include "render/device/VulkanRenderState.h"
#include "render/renderer/RenderGraph.h"
#include "render/renderer/SceneRenderer.h"
#include "ecs/components/Particles.h"
#include "ecs/systems/ParticleSystem.h"
#include "ecs/ECS.h"

using namespace Device;
using namespace ECS;

namespace render::effects {

	GPUParticles::~GPUParticles() = default;
	
	GPUParticles::GPUParticles(SceneRenderer& scene_renderer, Device::ShaderCache& shader_cache, ECS::EntityManager& manager)
		: scene_renderer(scene_renderer), manager(manager)
	{
		auto emit_shader_info = ShaderProgramInfo().AddShader(ShaderProgram::Stage::Compute, L"shaders/particles/ParticleSystem.hlsl", "EmitParticlesDirection");
		emit_shader = shader_cache.GetShaderProgram(emit_shader_info);

		auto update_shader_info = ShaderProgramInfo().AddShader(ShaderProgram::Stage::Compute, L"shaders/particles/ParticleSystem.hlsl", "UpdateParticles");
		update_shader = shader_cache.GetShaderProgram(update_shader_info);
	}

	void GPUParticles::Update(graph::RenderGraph& graph, float dt)
	{
		auto emitters = manager.GetChunkListsWithComponent<components::ParticleEmitter>();
		systems::GPUParticleUpdateSystem(manager, scene_renderer, *scene_renderer.GetDrawCallManager(), graph, *emit_shader, *update_shader).ProcessChunks(emitters);
	}

}