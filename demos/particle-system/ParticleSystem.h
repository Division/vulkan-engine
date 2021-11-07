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

namespace Device
{
	class GPUBuffer;
}

namespace Resources
{
	class TextureResource;
	class MultiMesh;
	class PhysCollider;
	class MaterialResource;
	class EntityResource;
	class SkeletonResource;
	class SkeletalAnimationResource;
}

class Game : public IGame {
public:
	Game();
	~Game();
	void init() override;
	void update(float dt) override;

private:
	void OnRender(const render::RenderCallbackData&, render::graph::RenderGraph& graph);

private:
	struct Handles;
	std::unique_ptr<Device::GPUBuffer> compute_buffer;
	Device::ShaderProgram* compute_program;
	std::unique_ptr<Handles> handles;
	std::unique_ptr<ViewerCamera> camera;

	ECS::EntityID box_id = 0;
	int32_t resolution = 0;

	ECS::EntityManager* manager = nullptr;

	Resources::Handle<Resources::EntityResource> animated_entity;

	Resources::Handle<Resources::SkeletalAnimationResource> animation;

	bool camera_control = false;
};
