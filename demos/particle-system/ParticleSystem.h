#pragma once

#include <vector>
#include "Engine.h"
#include "ecs/ECS.h"
#include "ecs/TransformGraph.h"
#include "ecs/systems/TransformSystem.h"
#include "ecs/systems/RendererSystem.h"
#include "ecs/components/Light.h"
#include "ecs/components/Physics.h"
#include "resources/ResourceCache.h"
#include "resources/MaterialResource.h"

class Mesh;
class Material;
class ViewerCamera;

namespace render
{
	class MaterialList;
	struct RenderCallbackData;
	
	namespace graph
	{
		class RenderGraph;
	}
}

namespace Vehicle::Utils
{
	class VehicleDataCache;
}

namespace ECS::components
{
	struct DeltaTime;
}

class Game : public IGame {
public:
	Game();
	~Game();
	void init() override;
	void update(float dt) override;

private:
	void OnRender(const render::RenderCallbackData&, render::graph::RenderGraph& graph);
	ECS::EntityID CreateSphere(vec4 color);
private:
	struct GPUResources;

	std::unique_ptr<GPUResources> gpu_resources;
	std::unique_ptr<ViewerCamera> camera;
	ECS::EntityID particle_system_id = 0;
	ECS::EntityManager* manager = nullptr;
	std::vector<ECS::EntityID> spheres;
	bool camera_control = false;
};
