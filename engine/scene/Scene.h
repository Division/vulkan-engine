#pragma once

#include "CommonIncludes.h"
#include "render/renderer/ICameraParamsProvider.h"
#include "IGame.h"
#include "render/shader/ShaderBufferStruct.h"

class Projector;
class LightObject;
class Camera;

namespace Physics
{
    class PhysXManager;
}

namespace ECS
{
	class EntityManager;
    class TransformGraph;

    typedef uint64_t EntityID;

    namespace components
    {
        class Light;
        class Projector;
        class Transform;
    }

    namespace systems
    {
        class NoChildTransformSystem;
        class RootTransformSystem;
        class UpdateRendererSystem;
        class PhysicsPostUpdateSystem;
    }
}

namespace render
{
    struct DebugSettings;
}

struct SceneLightData
{
    ECS::components::Light* light;
    ECS::components::Transform* transform;
    Device::ShaderBufferStruct::Light data;
    int index;
};

class Scene 
{
public:

    Scene(IGame& game, render::DebugSettings* settings);
    ~Scene();

    void Update(IGame& game, float dt);

    Camera* GetCamera() const { return camera.get(); }

    ECS::EntityManager* GetEntityManager() const { return entity_manager.get(); }
    ECS::TransformGraph* GetTransformGraph() const { return transform_graph.get(); }

    auto& GetVisibleLights() const { return visible_lights; }

    Physics::PhysXManager* GetPhysics() const { return physx_manager.get(); };

private:
    void ProcessPhysicsSystems();
    void ProcessTransformSystems();
    void ProcessRendererSystems();

private:
    render::DebugSettings* debug_settings;
    std::vector<SceneLightData> visible_lights;
    std::vector<ECS::components::Projector*> visible_projectors;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<Physics::PhysXManager> physx_manager;
    IGame* game;

    // ECS
    void OnEntityDestroyed(ECS::EntityID entity);

    std::unique_ptr<ECS::EntityManager> entity_manager;
    std::unique_ptr<ECS::TransformGraph> transform_graph;

    std::unique_ptr<ECS::systems::PhysicsPostUpdateSystem> physics_post_update_system;
    std::unique_ptr<ECS::systems::NoChildTransformSystem> no_child_system;
    std::unique_ptr<ECS::systems::RootTransformSystem> root_transform_system;
    std::unique_ptr<ECS::systems::UpdateRendererSystem> update_renderer_system;

};
