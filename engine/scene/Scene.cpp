#include "Scene.h"
#include "objects/Camera.h"
#include "ecs/ECS.h"
#include "ecs/TransformGraph.h"
#include "ecs/systems/RendererSystem.h"
#include "ecs/systems/TransformSystem.h"
#include "ecs/systems/UpdateDrawCallsSystem.h"
#include "ecs/systems/PhysicsSystem.h"
#include "ecs/systems/SkinningSystem.h"
#include "ecs/components/Static.h"
#include "ecs/components/MultiMeshRenderer.h"
#include "ecs/components/Light.h"
#include "ecs/components/Physics.h"
#include "ecs/components/AnimationController.h"
#include "render/renderer/DrawCallManager.h"
#include "render/debug/DebugSettings.h"
#include "render/debug/DebugDraw.h"
#include "Engine.h"
#include "Physics.h"
#include "resources/ResourceCache.h"

using namespace ECS;

Scene::~Scene() 
{

};

Scene::Scene(IGame& game, render::DebugSettings* settings)
    : debug_settings(settings), game(&game)
{
    camera = std::make_unique<Camera>();

    entity_manager = std::make_unique<EntityManager>();
    transform_graph = std::make_unique<TransformGraph>(*entity_manager);

    delta_time_static = std::make_unique<components::DeltaTime>();
    entity_manager->AddStaticComponent(delta_time_static.get());

    RegisterEngineComponentTemplates(*entity_manager);

    no_child_system = std::make_unique<systems::NoChildTransformSystem>(*transform_graph, *entity_manager);
    root_transform_system = std::make_unique<systems::RootTransformSystem>(*transform_graph, *entity_manager);
    update_renderer_system = std::make_unique<systems::UpdateRendererSystem>(*entity_manager);
    physics_post_update_system = std::make_unique<systems::PhysicsPostUpdateSystem>(*entity_manager);
    physx_manager = std::make_unique<Physics::PhysXManager>(game.GetPhysicsDelegate(), delta_time_static.get());
    skinning_system = std::make_unique<systems::SkinningSystem>(*entity_manager);
}

void Scene::Update(IGame& game, float dt)
{
    delta_time_static->dt = dt;
    // Game update
    game.update(dt);
    camera->Update();

    // Physics
    physx_manager->StepPhysics(dt);
    physx_manager->FetchResults();
    physx_manager->DrawDebug();

    auto animation_controllers = entity_manager->GetChunkListsWithComponent<components::AnimationController>();
    skinning_system->ProcessChunks(animation_controllers); // find a better place?

    // ECS update
    ProcessPhysicsSystems();
    ProcessTransformSystems();
    ProcessRendererSystems();

    DrawDebug();
}

void Scene::DrawDebug()
{
    if (debug_settings && debug_settings->draw_bounding_boxes)
    {
        auto transforms = entity_manager->GetChunkListsWithComponent<components::Transform>();
        CallbackSystem([](Chunk* chunk) {
            ComponentFetcher<components::Transform> transform_fetcher(*chunk);

            for (int i = 0; i < chunk->GetEntityCount(); i++)
            {
                auto* transform = transform_fetcher.GetComponent(i);
                Engine::Get()->GetDebugDraw()->DrawOBB(transform->GetOBB(), vec4(1, 1, 1, 1));
            }

            }, *entity_manager, false).ProcessChunks(transforms);
    }

    if (debug_settings && debug_settings->draw_skeletons)
    {
        auto controllers1 = entity_manager->GetChunkListsWithComponents<components::Transform, components::AnimationController>();
        systems::DebugDrawSkinningSystem(*entity_manager).ProcessChunks(controllers1);

        //auto controllers2 = entity_manager->GetChunkListsWithComponents<components::Transform, components::AnimationController>();
        //systems::DebugDrawSkinningVerticesSystem(*entity_manager).ProcessChunks(controllers2);
    }

    if (debug_settings && debug_settings->draw_lights)
    {
        auto lights = entity_manager->GetChunkListsWithComponents<components::Transform, components::Light>();
        CallbackSystem([](Chunk* chunk) {
            ComponentFetcher<components::Transform> transform_fetcher(*chunk);
            ComponentFetcher<components::Light> light_fetcher(*chunk);
            for (int i = 0; i < chunk->GetEntityCount(); i++)
            {
                auto* transform = transform_fetcher.GetComponent(i);
                auto* light = light_fetcher.GetComponent(i);
                Engine::Get()->GetDebugDraw()->DrawOBB(transform->GetOBB(), vec4(light->color, 1));
                Engine::Get()->GetDebugDraw()->DrawPoint(transform->WorldPosition(), light->color, 5.0f);
            }
        }, *entity_manager, false).ProcessChunks(lights);
    }
}

// ECS

void Scene::OnEntityDestroyed(EntityID entity)
{

}

void Scene::ProcessPhysicsSystems()
{
    // Update transform components with physics simulated data
    auto rigidbody_list = entity_manager->GetChunkListsWithComponents<components::RigidbodyDynamic, components::Transform>();
    physics_post_update_system->ProcessChunks(rigidbody_list);
}

void Scene::ProcessTransformSystems()
{
    // List of objects with top level transforms both with and without children
    auto top_level_list = entity_manager->GetChunkLists([](ChunkList* chunk_list) {
        auto child_hash = GetComponentHash<components::ChildTransform>();
        auto transform_hash = GetComponentHash<components::Transform>();
        return !chunk_list->HasComponent(child_hash) && chunk_list->HasComponent(transform_hash);
    });

    // List of objects with top level transforms that have children
    auto root_list = entity_manager->GetChunkListsWithComponents<components::RootTransform, components::Transform>();

    no_child_system->ProcessChunks(top_level_list);
    root_transform_system->ProcessChunks(root_list);
}

void Scene::ProcessRendererSystems()
{
    // Update lights
    auto lights = entity_manager->GetChunkListsWithComponents<components::Light, components::Transform>();

    visible_lights.clear();
    int index = 0;

    CallbackSystem([&](Chunk* chunk) {
        ComponentFetcher<components::Light> light_fetcher(*chunk);
        ComponentFetcher<components::Transform> transform_fetcher(*chunk);

        for (int i = 0; i < chunk->GetEntityCount(); i++)
        {
            auto* light = light_fetcher.GetComponent(i);
            auto* transform = transform_fetcher.GetComponent(i);
            light->UpdateMatrices(*transform);
            transform->bounds = AABB(transform->WorldPosition() - vec3(light->radius), transform->WorldPosition() + vec3(light->radius));
            if (camera->obbVisible(transform->GetOBB()))
            {
                SceneLightData data;
                data.position = transform->WorldPosition();
                data.direction = transform->Forward();
                data.light = light;
                data.index = index++;
                data.transform = transform;
                visible_lights.push_back(data);
            }
        }

    }, *entity_manager, false).ProcessChunks(lights);

    auto projectors = entity_manager->GetChunkListsWithComponents<components::Projector, components::Transform>();

    visible_projectors.clear();
    index = 0;

    /*CallbackSystem([&](Chunk* chunk) {
        ComponentFetcher<components::Projector> projector_fetcher(*chunk);
        ComponentFetcher<components::Transform> transform_fetcher(*chunk);

        for (int i = 0; i < chunk->GetEntityCount(); i++)
        {
            auto* projector = projector_fetcher.GetComponent(i);
            auto* transform = transform_fetcher.GetComponent(i);
            projector->UpdateMatrices(*transform);
            //transform->bounds = AABB(transform->WorldPosition() - vec3(projector->radius), transform->WorldPosition() + vec3(projector->radius));

            // TODO: obb from frustum

            if (camera->obbVisible(transform->GetOBB()))
            {
                SceneProjectorData data;
                data.data = projector->GetShaderStruct(transform->WorldPosition(), 1.0f);
                data.projector = projector;
                data.index = index++;
                data.transform = transform;
                visible_projectors.push_back(data);
            }
        }

        }, *entity_manager, false).ProcessChunks(lights);*/

    // TODO: add projectors

    // Append render data to MultiMeshRenderer component

    // Must have transform and either MeshRenderer or MultiMeshRenderer
    auto list = entity_manager->GetChunkLists([](ChunkList* chunk_list) {
        auto hash2 = GetComponentHash<components::MultiMeshRenderer>();
        auto transform_hash = GetComponentHash<components::Transform>();
        
        return chunk_list->HasComponent(transform_hash) && chunk_list->HasComponent(hash2);
    });

    // Updates object_params of the MeshRenderer
    update_renderer_system->ProcessChunks(list);
}

