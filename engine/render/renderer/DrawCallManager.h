#pragma once

#include "CommonIncludes.h"
#include "ecs/ECS.h"
#include <mutex>
#include "IRenderer.h"

namespace core 
{ 

	namespace ECS 
	{
		namespace components
		{
			struct DrawCall;
		}

		class EntityManager;
	}

	namespace Device
	{
		class ShaderProgram;
		class ShaderCache;
	}
}

class Material;
class Mesh;

namespace core { namespace render {

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
		DrawCallManager(SceneRenderer& scene_renderer);
		~DrawCallManager();

		ECS::EntityManager* GetManager() const { return manager.get(); }

		std::pair<ECS::EntityID, ECS::components::DrawCall*> AddDrawCall(const Mesh& mesh, const Material& material);
		void RemoveDrawCall(ECS::EntityID entity);

	private:
		std::mutex mutex;
		::core::Device::ShaderCache* shader_cache;
		std::unique_ptr<ECS::EntityManager> manager;
		SceneRenderer& scene_renderer;
		uint32_t depth_only_fragment_shader_hash;

		std::list<DrawCallList> draw_calls_lists;
	};

} }