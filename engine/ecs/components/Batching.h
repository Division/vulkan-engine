#include "render/mesh/Mesh.h"
#include "render/material/Material.h"
#include "resources/MultiMesh.h"
#include <atomic>

namespace ECS::systems
{
	class BatchingVolumeSystem;
}

namespace ECS::components
{

	struct BatchingVolume
	{

	public:
		struct JobData
		{
			enum class State
			{
				Idle,
				Processing,
				Ready
			};

			AABB aabb = AABB::Empty();
			Common::Handle<Resources::MultiMesh> result_mesh;
			render::MaterialList::Handle result_materials;
			std::atomic<State> state = State::Idle;
		};
	
		friend class ECS::systems::BatchingVolumeSystem;
	private:
		std::unique_ptr<JobData> job_data;

	public:

		BatchingVolume() : job_data(std::make_unique<JobData>()) {}
		~BatchingVolume() { while (IsProcessing()) { std::this_thread::yield(); } }
		BatchingVolume(BatchingVolume&&) = default;

		static uint32_t GetBatchHash(Material* material, Mesh* mesh)
		{
			const size_t material_ptr = (size_t)material;
			const std::pair<uint32_t, size_t> hashes = { mesh ? mesh->GetVertexLayout().GetHash() : 0u, material_ptr };
			return FastHash(&hashes, sizeof(hashes));
		}

		bool IsProcessing() const { return job_data->state.load() == JobData::State::Processing; }
		bool IsReady() const { return job_data->state.load() == JobData::State::Ready; }
		bool IsIdle() const { return job_data->state.load() == JobData::State::Idle; }

		struct BatchSrc
		{
			vec3 position = vec3(0);
			vec3 scale = vec3(1);
			quat rotation = quat();
			Mesh::Handle mesh;
			Material::Handle material;

			uint32_t GetHash() const { return GetBatchHash(material.get(), mesh.get()); }
		};

		std::vector<BatchSrc> src_meshes;
		bool is_dirty = true;
	};

}