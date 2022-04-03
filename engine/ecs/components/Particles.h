#pragma once

#include <stdint.h>
#include "render/Buffer/GPUBuffer.h"
#include "render/Buffer/DynamicBuffer.h"
#include "ecs/ECS.h"
#include <magic_enum/magic_enum.hpp>
#include "resources/MaterialResource.h"
#include "render/renderer/DrawCallManager.h"
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
			Initializer(uint32_t max_particles = 10000, render::MaterialUnion material = render::MaterialUnion());
			Initializer& SetShaderPath(std::wstring update_shader, std::wstring emit_shader = L"");
			Initializer& SetMaxParticles(uint32_t value) { max_particles = value; return *this; };
			Initializer& SetEmissionParams(EmissionParams value) { params = value; return *this; };
			Initializer& SetEmitterGeometry(EmitterGeometryUnion value) { emitter_geometry = value; return *this; };
			Initializer& SetMesh(Common::Handle<Mesh> value) { mesh = value; return *this; };
			Initializer& SetExtraBindings(ExtraBindings value) { extra_bindings = std::move(value); return *this; };

			ExtraBindings extra_bindings;
			std::wstring emit_shader_path;
			std::wstring update_shader_path;
			uint32_t max_particles = 10000;
			EmissionParams params = EmissionParams();
			render::MaterialUnion material = render::MaterialUnion();
			EmitterGeometryUnion emitter_geometry = EmitterGeometrySphere();
			Common::Handle<Mesh> mesh;
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

		void FlushEmitConstants(Device::ConstantBindings& bindings, vec3* emit_position);
		void FlushExtraConstants(Device::ConstantBindings& bindings);
		void FlushExtraResources(Device::ResourceBindings& bindings);

		ParticleEmitter& operator=(ParticleEmitter&&) = default;

#pragma pack(push, 1)
		struct CounterArgumentsData
		{
			/// DispatchIndirect args for Update()
			int32_t groupsX = 0;
			int32_t groupsY = 1;
			int32_t groupsZ = 1;
			//////////////////////////////////////

			/// DrawIndexedIndirect args for Render()
			int32_t indexCount = 6;
			int32_t instanceCount = 0;
			int32_t firstIndex = 0;
			int32_t vertexOffset = 0;
			int32_t firstInstance = 0;
			/////////////////////////////////////////

			// Persistent counters
			int32_t alive_count = 0; 
			int32_t dead_count = 0;
			int32_t alive_count_after_simulation = 0;
			//////////////////////

			CounterArgumentsData(int32_t capacity, int32_t index_count)
			{
				dead_count = capacity;
				indexCount = index_count;
			}
		};
#pragma pack(pop)

		struct GPUData
		{
			Device::GPUBuffer particles;
			Device::GPUBuffer dead_indices;
			Device::GPUBuffer alive_indices;
			Device::GPUBuffer draw_indices;
			Device::DynamicBuffer<uint8_t> counters;

			Device::GPUBuffer& GetAliveIndices() { return alive_indices; }
			Device::GPUBuffer& GetDrawIndices() { return draw_indices; }

			GPUData(uint32_t particle_count);
		};

		uint32_t id = 0;
		render::DrawCallManager::Handle draw_calls;
		render::MaterialUnion material;
		Common::Handle<Mesh> mesh;

		std::wstring emit_shader_path;
		std::wstring update_shader_path;

		Device::ShaderProgram* emit_shader = nullptr;
		Device::ShaderProgram* update_shader = nullptr;

		std::unique_ptr<GPUData> gpu_data;
		uint32_t max_particles = 10000;

		EmitterGeometryUnion emitter_geometry;
		EmissionParams emission_params;
		ExtraBindings extra_bindings;

		float time_since_last_emit = 0;
		bool emission_enabled = true;

		static void Initialize(EntityManager& manager, EntityID id, ParticleEmitter* emitter);

	private:
		Material::Handle actual_material; // Material copy with added particle buffers
	};
}