#include "BatchingSystem.h"
#include "ecs/components/Transform.h"
#include "ecs/components/MultiMeshRenderer.h"
#include <gsl/span>

namespace ECS::systems
{
	using namespace components;

	std::pair<Material::Handle, Mesh::Handle> CreateMergedMesh(gsl::span<BatchingVolume::BatchSrc*> batches)
	{
		assert(batches.size());

		std::vector<uint8_t> vertex_data;
		std::vector<uint8_t> index_data;

		uint32_t vertex_count = 0;
		uint32_t index_count = 0;

		const auto vertex_size = batches[0]->mesh->strideBytes();

		for (auto* batch : batches)
		{
			vertex_count += batch->mesh->vertexCount();
			index_count += batch->mesh->indexCount();
			assert(batch->mesh->strideBytes() == vertex_size);
		}

		assert(index_count % 3 == 0);

		vertex_data.resize(vertex_count * vertex_size);
		index_data.resize(index_count * sizeof(uint32_t));

		auto layout = Mesh::Layout(batches[0]->mesh->GetVertexLayout());

		size_t vertex_offset = 0;
		size_t index_offset = 0;

		auto aabb = AABB::Empty();

		float offset = 0;

		uint32_t vertex_counter = 0;
		for (auto* batch : batches)
		{
			memcpy(vertex_data.data() + vertex_offset, batch->mesh->GetVertexData().data(), batch->mesh->GetVertexData().size_bytes());
			auto transform = ComposeMatrix(batch->position, batch->rotation, batch->scale);
			auto normal_transform = glm::transpose(glm::inverse(transform));

			for (uint32_t i = 0; i < batch->mesh->vertexCount(); i++)
			{
				if (layout.HasPosition())
				{
					auto& position = layout.GetPosition(vertex_data.data() + vertex_offset, i);
					position = transform * vec4(position, 1);
					aabb.expand(position);
				}

				if (layout.HasNormal())
				{
					auto& normal_compressed = layout.GetNormal(vertex_data.data() + vertex_offset, i);
					vec3 normal = (vec4)normal_compressed;
					normal = glm::normalize(normal_transform * vec4(normal, 0));
					normal_compressed = Vector4_A2R10G10B10::FromSignedNormalizedFloat(vec4(normal, 0));
				}

				if (layout.HasTangent())
				{
					auto& tangent_compressed = layout.GetTangent(vertex_data.data() + vertex_offset, i);
					vec3 tangent = (vec4)tangent_compressed;
					tangent = glm::normalize(normal_transform * vec4(tangent, 0));
					tangent_compressed = Vector4_A2R10G10B10::FromSignedNormalizedFloat(vec4(tangent, 0));
				}
			}

			vertex_offset += batch->mesh->GetVertexData().size_bytes();

			auto indices = batch->mesh->GetIndexData();
			if (batch->mesh->UsesShortIndexes())
			{
				for (uint32_t i = 0; i < batch->mesh->indexCount(); i++)
					(uint32_t&)index_data[index_offset * sizeof(uint32_t) + i * sizeof(uint32_t)] = static_cast<uint32_t>(((uint16_t*)indices.data())[i]) + vertex_counter;
			}
			else
			{
				assert(indices.size_bytes() == batch->mesh->indexCount() * sizeof(uint32_t));
				for (uint32_t i = 0; i < batch->mesh->indexCount(); i++)
					(uint32_t&)index_data[index_offset * sizeof(uint32_t) + i * sizeof(uint32_t)] = ((uint32_t*)indices.data())[i] + vertex_counter;
			}

			index_offset += batch->mesh->indexCount();
			vertex_counter += batch->mesh->vertexCount();
		}

		assert(index_offset == index_count);

		if (Mesh::IsShortIndexCount(index_count))
		{
			for (uint32_t i = 0; i < index_count; i++)
				(uint16_t&)index_data[i * sizeof(uint16_t)] = static_cast<uint16_t>((uint32_t&)index_data[i * sizeof(uint32_t)]);
		}

		auto material = batches[0]->material;
		auto mesh = Mesh::Create(batches[0]->mesh->GetFlags(), vertex_data.data(), vertex_count, index_data.data(), index_count / 3, aabb, false, "BatchingVolume");

		return { material, mesh };
	}

	class BatchingVolumeSystem::ProcessBatchingJob : public Thread::Job
	{
	public:
		ProcessBatchingJob(std::vector<BatchingVolume::BatchSrc> src_meshes, BatchingVolume::JobData* data)
			: Job()
			, src_meshes(std::move(src_meshes))
			, data(data)
		{
			assert(data->state == BatchingVolume::JobData::State::Idle);
			data->state = BatchingVolume::JobData::State::Processing;
		}

		virtual void Execute() override
		{
			OPTICK_EVENT();
			BatchMap batch_map;

			for (auto& batch : src_meshes)
				batch_map.insert({ batch.GetHash(), &batch });

			utils::SmallVector<BatchingVolume::BatchSrc*, 128> batch_array;
			utils::SmallVector<Mesh::Handle, 128> mesh_array;

			auto material_list = render::MaterialList::Create();

			auto aabb = AABB::Empty();

			uint32_t key = batch_map.begin()->first;
			for (auto it = batch_map.begin(); it != batch_map.end(); it++)
			{
				const auto next = std::next(it);
				batch_array.push_back(it->second);

				if (next == batch_map.end() || key != next->first)
				{
					auto [material, mesh] = CreateMergedMesh(gsl::make_span(batch_array.data(), batch_array.size()));
					material_list->push_back(material);
					mesh_array.push_back(mesh);
					aabb.expand(mesh->aabb());

					key = it->first;
					batch_array.clear();
				}
			}

			data->result_materials = material_list;
			data->result_mesh = Resources::MultiMesh::Create(gsl::make_span(mesh_array.data(), mesh_array.size()), aabb);
			data->aabb = aabb;
			data->state = BatchingVolume::JobData::State::Ready;
		}

	private:
		std::vector<BatchingVolume::BatchSrc> src_meshes;
		BatchingVolume::JobData* data = nullptr;
	};

	void BatchingVolumeSystem::Process(Chunk* chunk)
	{
		OPTICK_EVENT();

		ComponentFetcher<MultiMeshRenderer> mesh_renderer_fetcher(*chunk);
		ComponentFetcher<Transform> transform_fetcher(*chunk);
		ComponentFetcher<BatchingVolume> batching_volume_fetcher(*chunk);


		for (int i = 0; i < chunk->GetEntityCount(); i++)
		{
			auto* batching_volume = batching_volume_fetcher.GetComponent(i);
			auto* transform = transform_fetcher.GetComponent(i);
			auto* mesh_renderer = mesh_renderer_fetcher.GetComponent(i);

			if (batching_volume->IsReady())
			{
				batching_volume->job_data->state = BatchingVolume::JobData::State::Idle;
				mesh_renderer->SetMultiMesh(batching_volume->job_data->result_mesh);
				mesh_renderer->materials = batching_volume->job_data->result_materials;
				mesh_renderer->material_resources.reset();
				transform->bounds = batching_volume->job_data->aabb;
			}

			if (!batching_volume->IsIdle() || !batching_volume->is_dirty)
				continue;


			if (batching_volume->src_meshes.size() > 0)
			{
				Thread::Scheduler::Get().SpawnJob<ProcessBatchingJob>(Thread::Job::Priority::High, batching_volume->src_meshes, batching_volume->job_data.get());
				//ProcessBatchingJob(batching_volume->src_meshes, batching_volume->job_data.get()).Execute();
			}
			else
			{
				mesh_renderer->draw_calls.Reset();
				if (mesh_renderer->materials)
					mesh_renderer->materials->clear();

				mesh_renderer->SetMultiMesh(Resources::MultiMesh::Handle(nullptr));
			}

			batching_volume->is_dirty = false;
		}
	}

}