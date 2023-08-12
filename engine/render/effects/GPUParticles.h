#pragma once

#include <memory>

namespace Device
{
	class Texture;
	class VulkanRenderState;
	class ShaderProgram;
	class ShaderCache;
	class ConstantBindings;
}

namespace ECS
{
	class EntityManager;
}

namespace ECS::systems
{
	class GPUParticleUpdateSystem;
}

namespace render::graph
{
	class RenderGraph;
	class IRenderPassBuilder;
}

namespace render
{
	class SceneRenderer;
	class DrawCallManager;
	class BitonicSort;
}

namespace render::effects {

	class GPUEmitterTemplate
	{
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

		struct Initializer
		{
			Initializer(uint32_t max_particles = 10000, bool sort = false, render::MaterialUnion material = render::MaterialUnion());
			Initializer& SetShaderPath(std::wstring update_shader, std::wstring emit_shader = L"");
			Initializer& SetMaxParticles(uint32_t value) { max_particles = value; return *this; };
			Initializer& SetMesh(Common::Handle<Mesh> value) { mesh = value; return *this; };
			Initializer& SetExtraBindings(ExtraBindings value) { extra_bindings = std::move(value); return *this; };

			bool sort;
			ExtraBindings extra_bindings;
			std::wstring emit_shader_path;
			std::wstring update_shader_path;
			uint32_t max_particles = 10000;
			render::MaterialUnion material = render::MaterialUnion();
			Common::Handle<Mesh> mesh;
		};
	};

	class GPUParticles
	{
	public:
		GPUParticles(SceneRenderer& scene_renderer, BitonicSort& bitonic_sort, ECS::EntityManager& manager);
		~GPUParticles();

		void Update(graph::RenderGraph& graph, const Device::ConstantBindings& global_constants, float dt);

	private:
		std::unique_ptr<ECS::systems::GPUParticleUpdateSystem> particle_update_system;
		SceneRenderer& scene_renderer;
		BitonicSort& bitonic_sort;
		ECS::EntityManager& manager;
	};

}