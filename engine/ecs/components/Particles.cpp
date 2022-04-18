#include "Particles.h"
#include "Engine.h"
#include "render/renderer/SceneRenderer.h"


namespace ECS::components
{

	ParticleEmitter::Initializer::Initializer(const render::GPUParticles::EmitterGPUData& gpu_data)
		: gpu_data(&gpu_data)
	{
	}

	ParticleEmitter::ParticleEmitter(const Initializer& initializer)
	{
		gpu_data = initializer.gpu_data;
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
		bindings.AddFloat2Binding(reinterpret_cast<vec2*>(&emission_params.speed), "emit_speed");
		bindings.AddFloat4Binding(&emission_params.color, "emit_color");
	}

	void ParticleEmitter::Initialize(EntityManager& manager, EntityID id, ParticleEmitter* emitter)
	{
		emitter->id = ++ParticleEmitter::id_counter;
	}

}