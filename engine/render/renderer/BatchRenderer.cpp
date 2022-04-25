#include "BatchRenderer.h"
#include "DrawCallManager.h"
#include "render/renderer/SceneBuffers.h"

class Mesh;

namespace render::BatchRenderer
{

	uint32_t GetDrawCallHash(const ECS::components::DrawCall* draw_call, bool is_depth)
	{
#pragma pack(push, 1)
		struct DrawCallHasher
		{
			const Mesh* mesh = nullptr;
			uint32_t shader_hash = 0;
			const void* descriptor_set = nullptr;
			uint32_t GetHash() const
			{
				return FastHash(this, sizeof(*this));
			}
		};
#pragma pack(pop)

		const auto* shader = is_depth ? draw_call->depth_only_shader : draw_call->shader;
		const auto* descriptor_set = is_depth ? draw_call->depth_only_descriptor_set : draw_call->descriptor_set;

		return DrawCallHasher{ draw_call->mesh, shader->GetHash(), descriptor_set }.GetHash();
	}

	bool DrawCallCanBeBatched(const ECS::components::DrawCall* draw_call)
	{
		return draw_call->indirect_buffer == nullptr && draw_call->instance_count == 1;
	}

	BatchList PrepareBatches(SceneBuffers& scene_buffers, const DrawCallList& draw_calls, bool is_depth)
	{
		BatchList result;

		auto* draw_call_instances = scene_buffers.GetDrawCallInstancesBuffer();

		for (uint32_t queue_index = 0; queue_index < (uint32_t)RenderQueue::Count; queue_index++)
		{
			std::unordered_map<uint32_t, Batch> batches;
			for (auto* draw_call : draw_calls.queues[queue_index])
			{
				if (DrawCallCanBeBatched(draw_call))
				{
					const auto draw_call_hash = GetDrawCallHash(draw_call, is_depth);

					auto it = batches.find(draw_call_hash);
					const bool should_add = it == batches.end();
					if (should_add)
						it = batches.insert({ draw_call_hash, {} }).first;

					auto& batch = it->second;
					batch.draw_calls.push_back(draw_call);
					// TODO: add material uniforms

					auto allocation = draw_call_instances->Allocate(sizeof(DrawCallInstance));
					if (should_add)
					{
						const auto* shader = is_depth ? draw_call->depth_only_shader : draw_call->shader;
						const auto* descriptor_set = is_depth ? draw_call->depth_only_descriptor_set : draw_call->descriptor_set;

						batch.mesh = draw_call->mesh;
						batch.hash = draw_call_hash;
						batch.index_offset = allocation.offset / sizeof(DrawCallInstance);
						batch.shader = shader;
						batch.descriptor_set = descriptor_set;
					}

					DrawCallInstance& instance_data = reinterpret_cast<DrawCallInstance&>(*allocation.pointer);
					instance_data = {};
					instance_data.material_index = 0; // TODO
					instance_data.skinning_index = 0; // TODO
					instance_data.model_matrix = draw_call->transform;
					instance_data.normal_matrix = draw_call->normal_transform;
					instance_data.aabb = AABB(draw_call->obb.min, draw_call->obb.max);
				}
				else
				{
					result.queues[queue_index].separate_draw_calls.push_back(draw_call);
				}
			}

			for (auto& pair : batches)
				result.queues[queue_index].batches.push_back(std::move(pair.second));
		}

		draw_call_instances->Upload();

		return result;
	}

}