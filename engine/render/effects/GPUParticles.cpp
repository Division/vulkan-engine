#include "GPUParticles.h"
#include "utils/MeshGeneration.h"
#include "render/shader/ShaderCache.h"
#include "render/device/VulkanRenderState.h"
#include "render/renderer/RenderGraph.h"
#include "render/renderer/SceneRenderer.h"
#include "ecs/components/Particles.h"
#include "ecs/systems/ParticleSystem.h"
#include "ecs/ECS.h"
#include "render/device/VulkanUtils.h"
#include "render/effects/BitonicSort.h"

using namespace Device;
using namespace ECS;

namespace render::GPUParticles {
	namespace
	{

		constexpr wchar_t* DEFAULT_SHADER_PATH = L"shaders/particles/particle_system_default.hlsl";

#pragma pack(push, 1)
		struct ParticleData
		{
			vec4 position;
			vec4 color;
			vec3 velocity;
			float life;
			vec3 size;
			float max_life;
		};
#pragma pack(pop)

		std::vector<uint32_t> GetSequentialIndices(uint32_t count)
		{
			std::vector<uint32_t> result(count);
			for (uint32_t i = 0; i < count; i++)
				result[i] = i;

			return result;
		}
	}
	EmitterGPUData::Initializer::Initializer(std::string name, uint32_t max_particles, bool sort, render::MaterialUnion material)
		: max_particles(max_particles)
		, sort(sort)
		, name(std::move(name))
	{
		this->material = material.Get();
		if (!this->material)
		{
			auto material = Material::Create();
			material->LightingEnabled(false);
			material->SetColor(vec4(1, 1, 1, 1));
			material->SetShaderPath(L"shaders/particles/particle_material_default.hlsl");
			material->SetRenderQueue(RenderQueue::Translucent);
			this->material = material;
		}
	}

	EmitterGPUData::Initializer& EmitterGPUData::Initializer::SetShaderPath(std::wstring update_shader, std::wstring emit_shader)
	{
		emit_shader_path = emit_shader;
		update_shader_path = update_shader;
		return *this;
	}

	EmitterGPUData::EmitterGPUData(const Initializer& initializer)
		: particles("particles data " + initializer.name, initializer.max_particles * sizeof(ParticleData), Device::BufferType::Storage)
		, dead_indices("particles dead indices " + initializer.name, initializer.max_particles * sizeof(uint32_t), Device::BufferType::Storage, GetSequentialIndices(initializer.max_particles).data())
		, alive_indices("particles alive indices " + initializer.name, initializer.max_particles * sizeof(uint32_t), (Device::BufferType)((uint32_t)Device::BufferType::Storage | (uint32_t)Device::BufferType::TransferDst))
		, draw_indices("particles draw indices " + initializer.name, initializer.max_particles * sizeof(uint32_t), (Device::BufferType)((uint32_t)Device::BufferType::Storage | (uint32_t)Device::BufferType::TransferSrc))
		, counters("particles counters " + initializer.name, sizeof(CounterArgumentsData),
			(Device::BufferType)((uint32_t)Device::BufferType::Indirect | (uint32_t)Device::BufferType::TransferDst | (uint32_t)Device::BufferType::TransferSrc),
			false, &CounterArgumentsData(initializer.max_particles, 6)
		)
		, sort(initializer.sort)
		, extra_bindings(initializer.extra_bindings)
		, mesh(initializer.mesh)
		, material(initializer.material)
		, max_particles(initializer.max_particles)
	{
		if (sort)
			sorted_indices = std::make_unique<Device::GPUBuffer>("particles sorted indices " + initializer.name, initializer.max_particles * sizeof(uvec2), (Device::BufferType)((uint32_t)Device::BufferType::Storage));
	
		if (!mesh)
		{
			mesh = Engine::Get()->GetSceneRenderer()->GetRendererResources().particle_quad_mesh;
		}

		const wchar_t* emit_shader_path = initializer.emit_shader_path.empty() ? DEFAULT_SHADER_PATH : initializer.emit_shader_path.c_str();
		const wchar_t* update_shader_path = initializer.update_shader_path.empty() ? DEFAULT_SHADER_PATH : initializer.update_shader_path.c_str();

		{
			auto shader_info = Device::ShaderProgramInfo().AddShader(Device::ShaderProgram::Stage::Compute, emit_shader_path, "EmitParticles", GetMacros());
			emit_shader = Engine::Get()->GetShaderCache()->GetShaderProgram(shader_info);
		}

		{
			auto shader_info = Device::ShaderProgramInfo().AddShader(Device::ShaderProgram::Stage::Compute, update_shader_path, "UpdateParticles", GetMacros());
			update_shader = Engine::Get()->GetShaderCache()->GetShaderProgram(shader_info);
		}

		if (!emit_shader)
			throw std::runtime_error("Emit shader not found");

		if (!update_shader)
			throw std::runtime_error("Update shader not found");
	}

	std::vector<Device::ShaderProgramInfo::Macro> EmitterGPUData::GetMacros() const
	{
		std::vector<Device::ShaderProgramInfo::Macro> result;

		return result;
	}

	void EmitterGPUData::FlushExtraConstants(Device::ConstantBindings& bindings) const
	{
		extra_bindings.constants.Flush(bindings);
	}

	void EmitterGPUData::FlushExtraResources(Device::ResourceBindings& bindings) const
	{
		extra_bindings.resources.Flush(bindings);

		bindings.AddBufferBinding("particles", particles.GetBuffer().get(), particles.GetSize());
		bindings.AddBufferBinding("dead_indices", dead_indices.GetBuffer().get(), dead_indices.GetSize());
		bindings.AddBufferBinding("alive_indices", GetAliveIndices().GetBuffer().get(), GetAliveIndices().GetSize());
		bindings.AddBufferBinding("draw_indices", GetDrawIndices().GetBuffer().get(), GetDrawIndices().GetSize());
		bindings.AddBufferBinding("counters", counters.GetBuffer().get(), counters.GetSize());
	}

	EmitterGPUData::~EmitterGPUData() = default;

	GPUParticles::~GPUParticles() = default;
	
	GPUParticles::GPUParticles(SceneRenderer& scene_renderer, BitonicSort& bitonic_sort, ECS::EntityManager& manager)
		: scene_renderer(scene_renderer), manager(manager), bitonic_sort(bitonic_sort)
	{
		particle_update_system = std::make_unique<systems::GPUParticleUpdateSystem>(manager);

		auto* shader_cache = scene_renderer.GetShaderCache();
		auto presort_args_shader_info = Device::ShaderProgramInfo().AddShader(Device::ShaderProgram::Stage::Compute, L"shaders/particles/particle_system_presort.hlsl", "PreSortDispatchArgs");
		pre_sort_args_shader = shader_cache->GetShaderProgram(presort_args_shader_info);

		auto presort_shader_info = Device::ShaderProgramInfo().AddShader(Device::ShaderProgram::Stage::Compute, L"shaders/particles/particle_system_presort.hlsl", "PreSortParticles");
		pre_sort_shader = shader_cache->GetShaderProgram(presort_shader_info);

		auto output_sorted_shader_info = Device::ShaderProgramInfo().AddShader(Device::ShaderProgram::Stage::Compute, L"shaders/particles/particle_system_presort.hlsl", "OutputSortedIndices");
		output_sorted_shader = shader_cache->GetShaderProgram(output_sorted_shader_info);
	}

	const EmitterGPUData* GPUParticles::CreateGPUData(const EmitterGPUData::Initializer& initializer) const
	{
		EmitterGPUData* result = new EmitterGPUData(initializer);

		gpu_datas.emplace(result);

		return result;
	}

	void GPUParticles::Update(graph::RenderGraph& graph, const Device::ConstantBindings& global_constants, float dt)
	{
		if (gpu_datas.empty())
			return;

		for (auto& gpu_data : gpu_datas)
			UpdateDrawCalls(*gpu_data, dt);

		struct Empty {};
		graph.AddPass<Empty>("ParticleSystems", render::profiler::ProfilerName::PassCompute, [&](render::graph::IRenderPassBuilder& builder)
			{
				builder.SetCompute(false);
				return Empty{};
			}, [this, &global_constants, dt](Device::VulkanRenderState& state)
			{
				// Copy
				for (auto& gpu_data : gpu_datas)
					ComputeParticlePreUpdate(*gpu_data, state);

				{
					const vk::MemoryBarrier barrier = vk::MemoryBarrier(vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead);
					state.Barrier({ &barrier, 1 }, vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader);
				}

				// Emit
				auto emitters = manager.GetChunkListsWithComponent<components::ParticleEmitter>();
				particle_update_system->ProcessChunks(emitters, global_constants, state);

				{
					const vk::MemoryBarrier barrier = vk::MemoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eIndirectCommandRead);
					state.Barrier({ &barrier, 1 }, vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader | vk::PipelineStageFlagBits::eDrawIndirect);
				}

				// Update
				for (auto& gpu_data : gpu_datas)
					ComputeParticleUpdate(*gpu_data, global_constants, state, dt);

				{
					const vk::MemoryBarrier barrier = vk::MemoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eIndirectCommandRead);
					state.Barrier({ &barrier, 1 }, vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eDrawIndirect);
				}
			}
		);
	}

	// TODO: remove draw calls if no particle emitter reference the data
	void GPUParticles::UpdateDrawCalls(EmitterGPUData& gpu_data, float dt)
	{
		auto& draw_call_manager = *scene_renderer.GetDrawCallManager();

		if (!gpu_data.draw_calls)
		{
			gpu_data.draw_calls = draw_call_manager.CreateHandle();

			auto actual_material = gpu_data.GetMaterial()->Clone();
			actual_material->AddExtraBuffer(gpu_data.GetParticles().GetBuffer(), "particles");
			actual_material->AddExtraBuffer(gpu_data.GetDrawIndices().GetBuffer(), "draw_indices");
			gpu_data.SetActualMaterial(actual_material);

			render::DrawCallInitializer initializer(*gpu_data.GetMesh(), *actual_material);
			auto draw_call = gpu_data.draw_calls.AddDrawCall(initializer);

			draw_call->queue = actual_material->GetRenderQueue();
			draw_call->indirect_buffer = gpu_data.GetCounters().GetBuffer().get();
			draw_call->indirect_buffer_offset = offsetof(CounterArgumentsData, indexCount);
			draw_call->transform = mat4();
			draw_call->obb = OBB(mat4(), vec3(-INFINITY), vec3(INFINITY));
		}

		CounterArgumentsData counters(0, gpu_data.GetMesh()->indexCount());
		constexpr size_t upload_size = offsetof(CounterArgumentsData, alive_count);
		// Reset counters buffer with default values
		auto* map = gpu_data.counters.Map();
		gpu_data.counters.SetUploadSize(upload_size);
		memcpy_s(map, upload_size, &counters, upload_size);
		gpu_data.counters.Unmap();
	}

	void GPUParticles::ComputeParticlePreUpdate(EmitterGPUData& gpu_data, Device::VulkanRenderState& state)
	{
		state.Copy(*gpu_data.GetDrawIndices().GetBuffer(), *gpu_data.GetAliveIndices().GetBuffer(), { 0, 0, gpu_data.GetAliveIndices().GetSize() });
		state.Copy(*gpu_data.GetCounters().GetBuffer(), *gpu_data.GetCounters().GetBuffer(), vk::BufferCopy(
			offsetof(CounterArgumentsData, alive_count_after_simulation),
			offsetof(CounterArgumentsData, alive_count),
			sizeof(uint32_t)
		));
	}

	void GPUParticles::ComputeParticleUpdate(EmitterGPUData& gpu_data, const Device::ConstantBindings& global_constants, Device::VulkanRenderState& state, float dt)
	{
		Device::ResourceBindings resources;
		Device::ConstantBindings constants;

		gpu_data.FlushExtraConstants(constants);
		gpu_data.FlushExtraResources(resources);

		constants.AddFloatBinding(&dt, "dt");

		state.DispatchIndirect(*gpu_data.GetUpdateShader(), resources, constants, *gpu_data.GetCounters().GetBuffer().get(), 0);

		if (gpu_data.GetSort())
		{
			const bool ascending = false;

			resources.AddBufferBinding("sort_indices", gpu_data.GetSortedIndices()->GetBuffer().get(), gpu_data.GetSortedIndices()->GetSize());

			Device::ConstantBindings sort_constants = global_constants;
			const uint32_t NullItem = ascending ? -1 : 0; // Descending sort
			const uint32_t CounterOffset = 0;
			sort_constants.AddUIntBinding(&NullItem, "NullItem");
			sort_constants.AddUIntBinding(&CounterOffset, "CounterOffset");
			{
				const vk::MemoryBarrier barrier = vk::MemoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead);
				state.Barrier({ &barrier, 1 }, vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader);
			}
			state.Dispatch(*pre_sort_args_shader, resources, sort_constants, uvec3(1,1,1));
			{
				const vk::MemoryBarrier barrier = vk::MemoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eIndirectCommandRead);
				state.Barrier({ &barrier, 1 }, vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eDrawIndirect);
			}

			const uint32_t pre_sort_dispatch_offset = offsetof(CounterArgumentsData, preSortGroupsX);
			state.DispatchIndirect(*pre_sort_shader, resources, sort_constants, *gpu_data.GetCounters().GetBuffer().get(), pre_sort_dispatch_offset);
			
			{
				const vk::MemoryBarrier barrier = vk::MemoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite);
				state.Barrier({ &barrier, 1 }, vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader);
			}

			const uint32_t alive_count_offset = offsetof(CounterArgumentsData, alive_count_after_simulation);
			bitonic_sort.Process(state, *gpu_data.GetSortedIndices()->GetBuffer().get(), *gpu_data.GetCounters().GetBuffer().get(), alive_count_offset, true, ascending);

			state.DispatchIndirect(*output_sorted_shader, resources, sort_constants, *gpu_data.GetCounters().GetBuffer().get(), 0);
			
			{
				const vk::MemoryBarrier barrier = vk::MemoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite);
				state.Barrier({ &barrier, 1 }, vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader);
			}
		}
	}

}