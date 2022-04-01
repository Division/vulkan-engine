#include "ParticleSystem.h"
#include "ecs/components/Particles.h"
#include "ecs/components/Static.h"
#include "ecs/components/Transform.h"
#include "render/renderer/RenderGraph.h"
#include "render/device/VulkanRenderState.h"
#include "render/renderer/SceneRenderer.h"
#include "render/mesh/Mesh.h"

using namespace ECS::components;

namespace ECS::systems
{
	void GPUParticleUpdateSystem::Process(Chunk* chunk)
	{
		OPTICK_EVENT();
		ComponentFetcher<ParticleEmitter> particle_emitter_fetcher(*chunk);
		ComponentFetcher<Transform> transform_fetcher(*chunk);

		struct PassInfo
		{
			render::graph::DependencyNode* particles = nullptr;
			render::graph::DependencyNode* dead_indices = nullptr;
			render::graph::DependencyNode* alive_indices = nullptr;
			render::graph::DependencyNode* draw_indices = nullptr;
			render::graph::DependencyNode* counters = nullptr;
		};

		auto dt = manager.GetStaticComponent<components::DeltaTime>()->dt;

		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{
			auto* particle_emitter = particle_emitter_fetcher.GetComponent(i);
			if (!particle_emitter->mesh || !particle_emitter->material)
				continue;

			auto* transform = transform_fetcher.GetComponent(i);

			if (!particle_emitter->draw_calls)
			{
				particle_emitter->draw_calls = draw_call_manager.CreateHandle();

				particle_emitter->actual_material = particle_emitter->material.Get()->Clone();
				particle_emitter->actual_material->AddExtraBuffer(particle_emitter->gpu_data->particles.GetBuffer(), "particles");
				particle_emitter->actual_material->AddExtraBuffer(particle_emitter->gpu_data->GetDrawIndices().GetBuffer(), "draw_indices");
				render::DrawCallInitializer initializer(*particle_emitter->mesh, *particle_emitter->actual_material);

				auto draw_call = particle_emitter->draw_calls.AddDrawCall(initializer);

				draw_call->queue = particle_emitter->material.Get()->GetRenderQueue();
				draw_call->indirect_buffer = particle_emitter->gpu_data->counters.GetBuffer();
				draw_call->indirect_buffer_offset = offsetof(components::ParticleEmitter::CounterArgumentsData, indexCount);
			}

			for (uint32_t i = 0; i < particle_emitter->draw_calls.GetDrawCallCount(); i++)
			{
				auto draw_call = particle_emitter->draw_calls.GetDrawCall(i);
				draw_call->transform = transform->GetLocalToWorld();
				draw_call->obb = transform->GetOBB();
			}

			components::ParticleEmitter::CounterArgumentsData counters(0, particle_emitter->mesh->indexCount());
			constexpr size_t upload_size = offsetof(components::ParticleEmitter::CounterArgumentsData, alive_count);
			// Reset counters buffer with default values
			auto* map = particle_emitter->gpu_data->counters.Map();
			particle_emitter->gpu_data->counters.SetUploadSize(upload_size);
			memcpy_s(map, upload_size, &counters, upload_size);
			particle_emitter->gpu_data->counters.Unmap();

			auto particles_wrapper = graph.RegisterBuffer(*particle_emitter->gpu_data->particles.GetBuffer().get());
			auto dead_indices_wrapper = graph.RegisterBuffer(*particle_emitter->gpu_data->dead_indices.GetBuffer().get());
			auto alive_indices_wrapper = graph.RegisterBuffer(*particle_emitter->gpu_data->GetAliveIndices().GetBuffer().get());
			auto draw_indices_wrapper = graph.RegisterBuffer(*particle_emitter->gpu_data->GetDrawIndices().GetBuffer().get());
			auto counters_wrapper = graph.RegisterBuffer(*particle_emitter->gpu_data->counters.GetBuffer());

			uint32_t emit_count = 0;
			if (particle_emitter->emission_enabled && particle_emitter->emission_params.emission_rate > 0)
			{
				particle_emitter->time_since_last_emit += dt;
				const float time_per_particle = 1.0f / (float)particle_emitter->emission_params.emission_rate;
				emit_count = floorf(particle_emitter->time_since_last_emit / time_per_particle);
				particle_emitter->time_since_last_emit -= emit_count * time_per_particle;
			}

			auto actual_emit_shader = particle_emitter->emit_shader ? particle_emitter->emit_shader : &emit_shader;
			auto emit_pass_info = graph.AddPass<PassInfo>("particles emit", render::profiler::ProfilerName::PassCompute, [&](render::graph::IRenderPassBuilder& builder)
			{
				builder.SetCompute(false);

				PassInfo result;
				result.particles = builder.AddOutput(*particles_wrapper);
				result.dead_indices = builder.AddOutput(*dead_indices_wrapper);
				result.alive_indices = builder.AddOutput(*alive_indices_wrapper);
				result.counters = builder.AddOutput(*counters_wrapper);
					
				return result;
			}, [shader = actual_emit_shader, particle_emitter, emit_count](Device::VulkanRenderState& state)
			{
				auto gpu_data = particle_emitter->gpu_data.get();

				Device::ResourceBindings bindings;
				const float time = Engine::Get()->time();
				float3 emit_position = vec3(0, 0, 0);
				float3 emit_direction = vec3(1, 0, 1);

				Device::ConstantBindings constants;
				constants.AddUIntBinding(&emit_count, "emit_count");
				constants.AddFloatBinding(&time, "time");
				particle_emitter->FlushEmitConstants(constants, &emit_position);

				bindings.AddBufferBinding("particles", gpu_data->particles.GetBuffer().get(), gpu_data->particles.GetSize());
				bindings.AddBufferBinding("dead_indices", gpu_data->dead_indices.GetBuffer().get(), gpu_data->dead_indices.GetSize());
				bindings.AddBufferBinding("alive_indices", gpu_data->GetAliveIndices().GetBuffer().get(), gpu_data->GetAliveIndices().GetSize());
				bindings.AddBufferBinding("counters", gpu_data->counters.GetBuffer(), gpu_data->counters.GetSize());

				const uint32_t threads = std::max(ceilf(emit_count / 64.0f), 1.0f);
				state.Dispatch(*shader, bindings, constants, uvec3(threads, 1, 1));
			});

			auto actual_update_shader = particle_emitter->update_shader ? particle_emitter->update_shader : &update_shader;
			auto update_pass_info = graph.AddPass<PassInfo>("particles update", render::profiler::ProfilerName::PassCompute, [&](render::graph::IRenderPassBuilder& builder)
			{
				builder.SetCompute(false);

				builder.AddInput(*emit_pass_info.particles);
				builder.AddInput(*emit_pass_info.dead_indices);
				builder.AddInput(*emit_pass_info.alive_indices);
				builder.AddInput(*emit_pass_info.counters);

				PassInfo result;
				result.particles = builder.AddOutput(*particles_wrapper);
				result.counters = builder.AddOutput(*counters_wrapper);
				result.draw_indices = builder.AddOutput(*draw_indices_wrapper);

				return result;
			}, [dt, shader = actual_update_shader, gpu_data = particle_emitter->gpu_data.get(), particle_emitter](Device::VulkanRenderState& state)
			{
				Device::ResourceBindings bindings;
				Device::ConstantBindings constants;

				const float duration_seconds = particle_emitter->emission_params.life.Max();
				const float lifetime_rate = 1.0f / duration_seconds;

				constants.AddFloatBinding(&dt, "dt");
				constants.AddFloatBinding(&lifetime_rate, "lifetime_rate");

				bindings.AddBufferBinding("particles", gpu_data->particles.GetBuffer().get(), gpu_data->particles.GetSize());
				bindings.AddBufferBinding("dead_indices", gpu_data->dead_indices.GetBuffer().get(), gpu_data->dead_indices.GetSize());
				bindings.AddBufferBinding("alive_indices", gpu_data->GetAliveIndices().GetBuffer().get(), gpu_data->GetAliveIndices().GetSize());
				bindings.AddBufferBinding("draw_indices", gpu_data->GetDrawIndices().GetBuffer().get(), gpu_data->GetDrawIndices().GetSize());
				bindings.AddBufferBinding("counters", gpu_data->counters.GetBuffer(), gpu_data->counters.GetSize());

				state.DispatchIndirect(*shader, bindings, constants, *gpu_data->counters.GetBuffer(), 0);
			});

			scene_renderer.AddUserRenderDependency(render::SceneRenderer::RenderDependencyType::Main, *update_pass_info.particles);
			scene_renderer.AddUserRenderDependency(render::SceneRenderer::RenderDependencyType::Main, *update_pass_info.counters);
			scene_renderer.AddUserRenderDependency(render::SceneRenderer::RenderDependencyType::Main, *update_pass_info.draw_indices);
		}
	}

}