#pragma once

#include <stdint.h>
#include "render/Buffer/GPUBuffer.h"
#include "render/Buffer/DynamicBuffer.h"
#include "ecs/ECS.h"
#include <magic_enum/magic_enum.hpp>
#include "resources/MaterialResource.h"
#include "render/material/Material.h"

namespace Device
{
	class ShaderProgram;
}

namespace render::graph
{
	struct ResourceWrapper;
	struct DependencyNode;
}

namespace ECS::systems
{
	class GPUParticleUpdateSystem;
}

namespace render::GPUParticles
{
	class EmitterGPUData;
}

class Mesh;

namespace ECS::components
{
	struct ParticleEmitter
	{
	private:
		static inline std::atomic_uint32_t instance_count;
		static inline std::atomic_uint32_t id_counter = 0;

	public:
		template<typename T> struct ParamRange : protected std::pair<T, T>
		{
			ParamRange(T min, T max) : std::pair<T, T>({ min, max }) {}
			ParamRange(T value = T()) : std::pair<T, T>({ value, value }) {}

			T& Min() { return first; };
			T& Max() { return second; };
		};

		struct EmitterGeometrySphere
		{
			float radius_min = 0;
			float radius_max = 1;
		};

		struct EmitterGeometryLine
		{
			vec3 offset;
		};

		enum class EmitterGeometryType
		{
			Sphere,
			Line,
		};

		typedef std::variant<EmitterGeometrySphere, EmitterGeometryLine> EmitterGeometryUnion;

		struct ExtraBindings
		{
			render::ResourceBindingStorage resources;
			render::ConstantBindingStorage constants;
		};

		struct EmissionParams
		{
			EmissionParams& SetEmissionRate(uint32_t value) { emission_rate = value; return *this; }
			EmissionParams& SetLife(ParamRange<float> value) { life = value; return *this; }
			EmissionParams& SetSize(ParamRange<float> value) { size = value; return *this; }
			EmissionParams& SetSpeed(ParamRange<float> value) { speed = value; return *this; }
			EmissionParams& SetDirection(vec3 value) { direction = value; return *this; }
			EmissionParams& SetConeAngle(float value) { cone_angle = value; return *this; }
			EmissionParams& SetColor(vec4 value) { color = value; return *this; }
			EmissionParams& SetAngle(ParamRange<float> value) { angle = value; return *this; }

			uint32_t emission_rate = 300;
			ParamRange<float> life = ParamRange<float>(0.5f, 1.0f);
			ParamRange<float> size = ParamRange<float>(float(1), float(1));
			ParamRange<float> speed = ParamRange<float>(float(1), float(1));
			ParamRange<float> angle = ParamRange<float>(0, M_PI * 2);
			vec3 direction = vec3(0, 1, 0);
			float cone_angle = M_PI / 8;
			vec4 color = vec4(1, 1, 1, 1);
		};

		struct Initializer
		{
			Initializer(const render::GPUParticles::EmitterGPUData& gpu_data);
			Initializer& SetEmissionParams(EmissionParams value) { params = value; return *this; };
			Initializer& SetEmitterGeometry(EmitterGeometryUnion value) { emitter_geometry = value; return *this; };

			EmissionParams params = EmissionParams();
			EmitterGeometryUnion emitter_geometry = EmitterGeometrySphere();
			const render::GPUParticles::EmitterGPUData* gpu_data = nullptr;
		};

		static_assert(std::variant_size_v<EmitterGeometryUnion> == magic_enum::enum_count<EmitterGeometryType>());
		EmitterGeometryType GetEmitterGeometryType() const { return (EmitterGeometryType)emitter_geometry.index(); }

		static uint32_t GetCount() { return instance_count.load(); }

		friend class systems::GPUParticleUpdateSystem;

		std::vector<Device::ShaderProgramInfo::Macro> GetMacros() const;

		ParticleEmitter(const Initializer& initializer);

		~ParticleEmitter()
		{
			instance_count--;
		}

		ParticleEmitter(ParticleEmitter&& other)
		{
			*this = std::move(other);
			instance_count++;
		}

		const render::GPUParticles::EmitterGPUData& GetGPUData() const { return *gpu_data; }

		void FlushEmitConstants(Device::ConstantBindings& bindings, vec3* emit_position);

		ParticleEmitter& operator=(ParticleEmitter&&) = default;

		uint32_t id = 0;
		

		EmitterGeometryUnion emitter_geometry;
		EmissionParams emission_params;

		float time_since_last_emit = 0;
		bool emission_enabled = true;

		static void Initialize(EntityManager& manager, EntityID id, ParticleEmitter* emitter);

	private:
		const render::GPUParticles::EmitterGPUData* gpu_data = nullptr;
		Material::Handle actual_material; // Material copy with added particle buffers
	};
}