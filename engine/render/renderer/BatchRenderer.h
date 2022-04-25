#pragma once

#include "IRenderer.h"

namespace render
{
	struct DrawCallList;
	class SceneBuffers;
}

namespace ECS::components
{
	struct DrawCall;
}

class Mesh;

namespace Device
{
	class ShaderProgram;
	class DescriptorSet;
}

namespace render::BatchRenderer
{

#pragma pack(push, 1)
	struct DrawCallInstance
	{
		mat4 model_matrix;
		mat4 normal_matrix;
		AABB aabb;
		uint32_t material_index;
		uint32_t skinning_index;
		uvec2 filler;
	};
#pragma pack(pop)

	struct Batch
	{
		uint32_t index_offset = 0;
		uint32_t hash = 0;
		const Mesh* mesh = nullptr;
		const Device::ShaderProgram* shader = nullptr;
		const Device::DescriptorSet* descriptor_set = nullptr;

		std::vector<const ECS::components::DrawCall*> draw_calls;
	};

	struct BatchList
	{
		struct QueueBatches
		{
			std::vector<Batch> batches;
			std::vector<const ECS::components::DrawCall*> separate_draw_calls; // draw calls that can't be batched
		};

		std::array<QueueBatches, (int)RenderQueue::Count> queues;
	};

	BatchList PrepareBatches(SceneBuffers& scene_buffers, const DrawCallList& draw_calls,  bool is_depth);

}