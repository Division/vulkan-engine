#pragma once

#include "ecs/System.h"
#include "render/renderer/IRenderer.h"

namespace render
{
	class SceneRenderer;
	class DrawCallManager;
	class BitonicSort;
}

namespace Device
{
	class ShaderProgram;
	class VulkanRenderState;
	class ConstantBindings;
}

namespace ECS::systems
{
	class GPUParticleUpdateSystem : public System
	{
		Device::VulkanRenderState* state = nullptr;
		const Device::ConstantBindings* global_constants = nullptr;

		void ProcessChunks(const ChunkList::List& list) override
		{
			System::ProcessChunks(list);
		}

	public:
		GPUParticleUpdateSystem(EntityManager& manager);
		~GPUParticleUpdateSystem();

		void ProcessChunks(const ChunkList::List& list, const Device::ConstantBindings& global_constants, Device::VulkanRenderState& state);

		void Process(Chunk* chunk) override;
	};

}