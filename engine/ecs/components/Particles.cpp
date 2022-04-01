#include "Particles.h"
#include "Engine.h"
#include "render/renderer/SceneRenderer.h"

namespace ECS::components
{
	namespace
	{
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

	ParticleEmitter::Initializer::Initializer(uint32_t max_particles, render::MaterialUnion material)
		: max_particles(max_particles)
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

	ParticleEmitter::GPUData::GPUData(uint32_t particle_count)
		: particles("particles data", particle_count * sizeof(ParticleData), Device::BufferType::Storage)
		, dead_indices("particles dead indices", particle_count * sizeof(uint32_t), Device::BufferType::Storage, GetSequentialIndices(particle_count).data())
		, alive_indices("particles alive indices", particle_count * sizeof(uint32_t), Device::BufferType::Storage)
		, draw_indices("particles draw indices", particle_count * sizeof(uint32_t), Device::BufferType::Storage)
		, counters("particles counters", sizeof(CounterArgumentsData), Device::BufferType::Indirect, false, &CounterArgumentsData(particle_count, 6))
	{
	}

	ParticleEmitter::ParticleEmitter(const Initializer& initializer)
	{
		max_particles = initializer.max_particles;
		material = initializer.material;
		mesh = initializer.mesh;

		emit_shader_path = initializer.emit_shader_path;
		update_shader_path = initializer.update_shader_path;
		emitter_geometry = initializer.emitter_geometry;

		emission_params = initializer.params;

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
		bindings.AddFloat4Binding(&emission_params.color, "emit_color");
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

		emitter->gpu_data = std::make_unique<GPUData>(emitter->max_particles);

		std::vector<Device::ShaderProgramInfo::Macro> defines;

		if (!emitter->mesh)
		{
			emitter->mesh = Engine::Get()->GetSceneRenderer()->GetRendererResources().particle_quad_mesh;
		}

		if (!emitter->emit_shader_path.empty())
		{
			auto shader_info = Device::ShaderProgramInfo().AddShader(Device::ShaderProgram::Stage::Compute, emitter->emit_shader_path, "EmitParticles", emitter->GetMacros());
			emitter->emit_shader = Engine::Get()->GetShaderCache()->GetShaderProgram(shader_info);
		}

		if (!emitter->update_shader_path.empty())
		{
			auto shader_info = Device::ShaderProgramInfo().AddShader(Device::ShaderProgram::Stage::Compute, emitter->update_shader_path, "UpdateParticles", emitter->GetMacros());
			emitter->update_shader = Engine::Get()->GetShaderCache()->GetShaderProgram(shader_info);
		}
	}

}