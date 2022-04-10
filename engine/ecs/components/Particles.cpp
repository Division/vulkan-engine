#include "Particles.h"
#include "Engine.h"
#include "render/renderer/SceneRenderer.h"

namespace ECS::components
{
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

	ParticleEmitter::Initializer::Initializer(uint32_t max_particles, bool sort, render::MaterialUnion material)
		: max_particles(max_particles)
		, sort(sort)
		, params(params)
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

	ParticleEmitter::Initializer& ParticleEmitter::Initializer::SetShaderPath(std::wstring update_shader, std::wstring emit_shader)
	{
		emit_shader_path = emit_shader;
		update_shader_path = update_shader;
		return *this;
	}

	ParticleEmitter::GPUData::GPUData(uint32_t particle_count, bool sort)
		: particles("particles data", particle_count * sizeof(ParticleData), Device::BufferType::Storage)
		, dead_indices("particles dead indices", particle_count * sizeof(uint32_t), Device::BufferType::Storage, GetSequentialIndices(particle_count).data())
		, alive_indices("particles alive indices", particle_count * sizeof(uint32_t), (Device::BufferType)((uint32_t)Device::BufferType::Storage | (uint32_t)Device::BufferType::TransferDst))
		, draw_indices("particles draw indices", particle_count * sizeof(uint32_t), (Device::BufferType)((uint32_t)Device::BufferType::Storage | (uint32_t)Device::BufferType::TransferSrc))
		, counters("particles counters", sizeof(CounterArgumentsData), 
			(Device::BufferType)((uint32_t)Device::BufferType::Indirect | (uint32_t)Device::BufferType::TransferDst | (uint32_t)Device::BufferType::TransferSrc), 
			false, &CounterArgumentsData(particle_count, 6)
		)
	{
		if (sort)
			sorted_indices = std::make_unique<Device::GPUBuffer>("particles sorted indices", particle_count * sizeof(uvec2), (Device::BufferType)((uint32_t)Device::BufferType::Storage));
	}

	ParticleEmitter::ParticleEmitter(const Initializer& initializer)
	{
		max_particles = initializer.max_particles;
		material = initializer.material;
		mesh = initializer.mesh;
		sort = initializer.sort;

		emit_shader_path = initializer.emit_shader_path;
		update_shader_path = initializer.update_shader_path;
		emitter_geometry = initializer.emitter_geometry;

		emission_params = initializer.params;
		extra_bindings = initializer.extra_bindings;

		instance_count++;
	}

	void ParticleEmitter::FlushEmitConstants(Device::ConstantBindings& bindings, vec3* emit_position)
	{
		switch (GetEmitterGeometryType())
		{
		case EmitterGeometryType::Sphere:
			bindings.AddFloatBinding(&std::get<(size_t)EmitterGeometryType::Sphere>(emitter_geometry).radius_min, "emit_radius");
			break;
		case EmitterGeometryType::Line:
			bindings.AddFloat3Binding(&std::get<(size_t)EmitterGeometryType::Line>(emitter_geometry).offset, "emit_offset");
			break;
		}

		bindings.AddUIntBinding(&id, "emitter_id");
		bindings.AddFloat3Binding(emit_position, "emit_position");
		bindings.AddFloat3Binding(&emission_params.direction, "emit_direction");
		bindings.AddFloatBinding(&emission_params.cone_angle, "emit_cone_angle");
		bindings.AddFloat2Binding(reinterpret_cast<vec2*>(&emission_params.size), "emit_size");
		bindings.AddFloat2Binding(reinterpret_cast<vec2*>(&emission_params.angle), "emit_angle");
		bindings.AddFloat2Binding(reinterpret_cast<vec2*>(&emission_params.life), "emit_life");
		bindings.AddFloat2Binding(reinterpret_cast<vec2*>(&emission_params.speed), "emit_speed");
		bindings.AddFloat4Binding(&emission_params.color, "emit_color");
	}

	void ParticleEmitter::FlushExtraConstants(Device::ConstantBindings& bindings)
	{
		extra_bindings.constants.Flush(bindings);
	}

	void ParticleEmitter::FlushExtraResources(Device::ResourceBindings& bindings)
	{
		extra_bindings.resources.Flush(bindings);
	}

	std::vector<Device::ShaderProgramInfo::Macro> ParticleEmitter::GetMacros() const
	{
		std::vector<Device::ShaderProgramInfo::Macro> result;

		switch (GetEmitterGeometryType())
		{
			case EmitterGeometryType::Line	 : result.push_back({ "EMITTER_GEOMETRY_LINE", "1" });   break;
			case EmitterGeometryType::Sphere : result.push_back({ "EMITTER_GEOMETRY_SPHERE", "1" }); break;
		}

		return result;
	}

	void ParticleEmitter::Initialize(EntityManager& manager, EntityID id, ParticleEmitter* emitter)
	{
		emitter->id = ++ParticleEmitter::id_counter;

		emitter->gpu_data = std::make_unique<GPUData>(emitter->max_particles, emitter->sort);

		std::vector<Device::ShaderProgramInfo::Macro> defines;

		if (!emitter->mesh)
		{
			emitter->mesh = Engine::Get()->GetSceneRenderer()->GetRendererResources().particle_quad_mesh;
		}

		const wchar_t* emit_shader_path = emitter->emit_shader_path.empty() ? DEFAULT_SHADER_PATH : emitter->emit_shader_path.c_str();
		const wchar_t* update_shader_path = emitter->update_shader_path.empty() ? DEFAULT_SHADER_PATH : emitter->update_shader_path.c_str();

		{
			auto shader_info = Device::ShaderProgramInfo().AddShader(Device::ShaderProgram::Stage::Compute, emit_shader_path, "EmitParticles", emitter->GetMacros());
			emitter->emit_shader = Engine::Get()->GetShaderCache()->GetShaderProgram(shader_info);
		}

		{
			auto shader_info = Device::ShaderProgramInfo().AddShader(Device::ShaderProgram::Stage::Compute, update_shader_path, "UpdateParticles", emitter->GetMacros());
			emitter->update_shader = Engine::Get()->GetShaderCache()->GetShaderProgram(shader_info);
		}

		if (!emitter->emit_shader)
			throw std::runtime_error("Emit shader not found");

		if (!emitter->update_shader)
			throw std::runtime_error("Update shader not found");
	}

}