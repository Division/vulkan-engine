#pragma once

#include <deque>
#include "CommonIncludes.h"
#include "ecs/ECS.h"
#include <mutex>
#include "IRenderer.h"
#include "render/shader/ShaderCache.h"
#include "ecs/components/DrawCall.h"

namespace ECS 
{
	namespace components
	{
		struct DrawCall;
	}

	class EntityManager;
	class CommandBuffer;
}

namespace Device
{
	class ShaderProgram;
	class ShaderCache;
}

namespace utils
{
	template<typename T> class Pool;
}


class Material;
class Mesh;

namespace render {

	class SceneRenderer;

	struct DrawCallList
	{
		std::array<std::vector<ECS::components::DrawCall*>, (int)RenderQueue::Count> queues;
		
		void Clear() 
		{
			for (auto& queue : queues)
				queue.clear();
		}
	};

	struct DrawCallInitializer
	{
		DrawCallInitializer(const Mesh& mesh, const Material& material)
			: mesh(mesh), material(material)
		{}

		DrawCallInitializer& SetHasSkinning(bool value) { has_skinning = value; return *this; }
		DrawCallInitializer& SetResources(const Device::ResourceBindings& value) { resources = value; return *this; }

		const Mesh& const mesh;
		const Material& const material;
		Device::ResourceBindings resources; // Draw call may specify extra resources in addition to material and global ones
		bool has_skinning = false;
	};

	class DrawCallManager
	{
	public:

		// Manages lifetime of multiple draw calls. Deletes it's draw calls when destroyed.
		// returned DrawCall* pointers are guaranteed to be valid during Handle lifetime
		class Handle
		{
		public:
			ECS::components::DrawCall* AddDrawCall(const DrawCallInitializer& initializer);
			bool RemoveDrawCall(ECS::EntityID draw_call);
			void RemoveAllDrawCalls();

			~Handle();
			Handle() : manager(nullptr) {}
			Handle(Handle&& other);
			Handle(const Handle&) = delete;
			explicit operator bool() const { return manager != nullptr; }

			Handle& operator=(Handle&& other);

			size_t GetDrawCallCount() const { return draw_calls.size(); }
			ECS::components::DrawCall* GetDrawCall(size_t index) const 
			{ 
				return manager->GetManager()->GetComponent<ECS::components::DrawCall>(draw_calls[index]);
			}

			ECS::components::SkinningData* GetSkinningData(size_t index) const
			{
				return manager->GetManager()->GetComponent<ECS::components::SkinningData>(draw_calls[index]);
			}

			void Reset();

		private:
			friend class DrawCallManager;
			Handle(DrawCallManager& manager) : manager(&manager) {};
			
		private:
			utils::SmallVector<ECS::EntityID, 4> draw_calls;
			DrawCallManager* manager;
		};

		DrawCallManager(SceneRenderer& scene_renderer);
		~DrawCallManager();

		ECS::EntityManager* GetManager() const { return manager.get(); }

		Handle CreateHandle() { return Handle(*this); }

		std::pair<ECS::EntityID, ECS::components::DrawCall*> AddDrawCall(const DrawCallInitializer& initializer);
		void RemoveDrawCall(ECS::EntityID entity);
		ECS::ChunkList::List GetDrawCallChunks() const;
		ECS::ChunkList::List GetSkinningChunks() const;

		DrawCallList* ObtaintDrawCallList();
		void ReleaseDrawCallLists();

		void Update();

	private:
		::Device::ShaderCache* shader_cache;
		std::unique_ptr<ECS::EntityManager> manager;
		std::unique_ptr<ECS::CommandBuffer> command_buffer;
		SceneRenderer& scene_renderer;

		std::unique_ptr<utils::Pool<DrawCallList>> draw_call_list_pool;
		std::vector<std::unique_ptr<DrawCallList>> obtained_draw_call_lists;
	};

}