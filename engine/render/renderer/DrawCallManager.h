#pragma once

#include <deque>
#include "CommonIncludes.h"
#include "ecs/ECS.h"
#include <mutex>
#include "IRenderer.h"
#include "render/shader/ShaderCache.h"

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


	class DrawCallManager
	{
	public:

		// Manages lifetime of multiple draw calls. Deletes it's draw calls when destroyed.
		// returned DrawCall* pointers are guaranteed to be valid during Handle lifetime
		class Handle
		{
		public:
			ECS::components::DrawCall* AddDrawCall(const Mesh& mesh, const Material& material);
			bool RemoveDrawCall(ECS::components::DrawCall*);
			void RemoveAllDrawCalls();

			~Handle();
			Handle() : manager(nullptr) {}
			Handle(Handle&& other);
			Handle(const Handle&) = delete;
			explicit operator bool() const { return manager != nullptr; }

			Handle& operator=(Handle&& other);

		private:
			friend class DrawCallManager;
			Handle(DrawCallManager& manager) : manager(&manager) {};
			
		private:
			utils::SmallVector<std::pair<ECS::EntityID, ECS::components::DrawCall*>, 4> draw_calls;
			DrawCallManager* manager;
		};

		DrawCallManager(SceneRenderer& scene_renderer);
		~DrawCallManager();

		ECS::EntityManager* GetManager() const { return manager.get(); }

		Handle CreateHandle() { return Handle(*this); }

		std::pair<ECS::EntityID, ECS::components::DrawCall*> AddDrawCall(const Mesh& mesh, const Material& material);
		void RemoveDrawCall(ECS::EntityID entity);
		ECS::ChunkList::List GetDrawCallChunks();

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