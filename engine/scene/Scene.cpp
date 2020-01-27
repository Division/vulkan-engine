//
// Created by Sidorenko Nikita on 3/28/18.
//

#include "Scene.h"
#include "objects/Projector.h"
#include "objects/Camera.h"
#include "objects/LightObject.h"
#include "ecs/ECS.h"
#include "ecs/TransformGraph.h"
#include "ecs/systems/RendererSystem.h"
#include "ecs/systems/TransformSystem.h"
#include "ecs/systems/UpdateDrawCallsSystem.h"
#include "ecs/components/MeshRenderer.h"
#include "render/renderer/DrawCallManager.h"

#define IS_CAMERA(object) (bool)(dynamic_cast<Camera *>((object).get()))
#define IS_LIGHT(object) (bool)(dynamic_cast<LightObject *>((object).get()))
#define IS_PROJECTOR(object) (bool)(dynamic_cast<Projector *>((object).get()))

using namespace core::ECS;

void Scene::setAsDefault() {
  GameObject::_defaultManager = this;
}

Scene::Scene() {
    if (!GameObject::_defaultManager) {
        setAsDefault();
    }

    camera = CreateGameObject<Camera>();

    entity_manager = std::make_unique<EntityManager>();
    transform_graph = std::make_unique<TransformGraph>(*entity_manager);

    no_child_system = std::make_unique<systems::NoChildTransformSystem>(*transform_graph, *entity_manager);
    root_transform_system = std::make_unique<systems::RootTransformSystem>(*transform_graph, *entity_manager);
    update_renderer_system = std::make_unique<systems::UpdateRendererSystem>(*entity_manager);

    entity_manager->AddEntityDestroyCallback(std::bind(&Scene::OnEntityDestroyed, this, std::placeholders::_1));
}

void Scene::addGameObject(GameObjectPtr object) {
  if (_objectMap.find(object->id()) == _objectMap.end()) {
    _gameObjects.push_back(object);
    _objectMap[object->id()] = object;

    object->_setManager(this);
    this->transformChangeParent(object->transform(), nullptr, nullptr);

    this->_processAddedObject(object);
  }
}

void Scene::_processAddedObject(GameObjectPtr object) {
  _startList.push_back(object);

  // Object is light
  if (IS_LIGHT(object)) {
    LightObjectPtr light = std::dynamic_pointer_cast<LightObject>(object);
    _lights.push_back(light); // TODO: try to find free empty index
    _lightCount += 1;
  }

    // Object is projector
  else if (IS_PROJECTOR(object)) {
    auto projector = std::dynamic_pointer_cast<Projector>(object);
    _projectors.push_back(projector);
  }
}

void Scene::_processRemovedObject(GameObjectPtr object) {
  // Object is camera
//  if (IS_CAMERA(object)) {
//  }

  if (IS_LIGHT(object)) {
    for (auto &light : _lights) {
      if (light->id() == object->id()) {
        light = nullptr; // just set to null for future reuse
        _lightCount -= 1;
        break;
      }
    }
  }

  else if (IS_PROJECTOR(object)) {
    for (auto &projector : _projectors) {
      if (projector->id() == object->id()) {
        projector = nullptr;
        break;
      }
    }
  }

  object->_destroy();
}

// TODO: destroy out of update loop
void Scene::destroyGameObject(GameObjectPtr object) {
  if (object->destroyed()) {
    ENGLog("ERROR: attempt to destroy already destroyed object");
    return;
  }

  if (_objectMap.find(object->id()) != _objectMap.end()) {
    _objectMap.erase(object->id());
    object->transform()->setParent(nullptr);

    auto foundObject = std::find_if(_gameObjects.begin(), _gameObjects.end(), [&object](const GameObjectPtr &obj) -> bool {
      return object->id() == obj->id();
    });

    if (foundObject == _gameObjects.end()) {
      std::string error = "ERROR destroying object, not found in the list";
      ENGLog("%s", error.c_str());
      throw error;
    }

    auto index = foundObject - _gameObjects.begin();
    _gameObjects[index] = _gameObjects[_gameObjects.size() - 1];
    _gameObjects.pop_back();

    _processRemovedObject(object);
  }
}

void Scene::transformChangeParent(TransformPtr transform, TransformPtr oldParent, TransformPtr newParent) {
  if (oldParent) {
    oldParent->_removeChild(transform);
  }

  if (newParent) {
    newParent->_addChild(transform);
    _rootTransformMap.erase(transform->gameObject()->id());
  } else {
    _rootTransformMap[transform->gameObject()->id()] = transform;
  }

  transform->_parent = newParent;
}

void Scene::update(float dt) {
  if (!_startList.empty()) {
	auto start_list = _startList;
    for (auto &object : start_list) {
      object->start();
    }
    _startList.clear();
  }

  for (auto &object : _gameObjects) {
    if (object->active()) {
      object->update(dt);
    }
  }

  for (auto &object : _gameObjects) {
    if (object->active()) {
//      getEngine()->debugDraw()->drawOBB({ object->transform()->worldMatrix(), object->cullingData().bounds.min, object->cullingData().bounds.max }, vec4(1, 1, 0, 1));
      object->_processAnimations(dt);
    }
  }

  _updateTransforms();

  for (auto &object : _gameObjects) {
    if (object->active()) {
      object->postUpdate();
    }
  }

  for (auto &iterator : _visibilityMap) {
    iterator.second.hasData = false;
  }

  // ECS update
  ProcessTransformSystems();
  ProcessRendererSystems();
}

void Scene::_updateTransforms() {
  for (auto &transform : _rootTransformMap) {
    transform.second->_updateTransform(nullptr, false);
  }
}

Scene::Visibility &Scene::_getVisibilityForCamera(const ICameraParamsProvider* camera) const {
  auto &visibility = _visibilityMap[camera];
  visibility.hasData = true;
  visibility.projectors.clear();
  visibility.lights.clear();
  visibility.objects.clear();

  auto &frustum = camera->frustum();
  unsigned int counter = 0;
  for (auto &object : _projectors) {
    auto layerVisible = (bool)(object->layer() & camera->cameraVisibilityMask());
    if (!layerVisible || !_objectIsVisible(object, frustum)) { continue; }
    object->_index = counter++;
    visibility.projectors.push_back(object);
  }

  counter = 0;
  for (auto &object : _lights) {
    auto layerVisible = (bool)(object->layer() & camera->cameraVisibilityMask());
    if (!layerVisible || !_objectIsVisible(object, frustum)) { continue; }
    object->_index = counter++;
    visibility.lights.push_back(object);
  }

//  ENGLog("visible lights: %i, projectors: %i", visibility.lights.size(), visibility.projectors.size());

  for (auto &object : _gameObjects) {
    auto layerVisible = (bool)(object->layer() & camera->cameraVisibilityMask());
    if (!layerVisible || !object->isRenderable() || !_objectIsVisible(object, frustum)) { continue; }
    visibility.objects.push_back(object);
  }

  return visibility;
}

// ECS

void Scene::OnEntityDestroyed(EntityID entity)
{

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
    // Append render data to MeshRenderer component
    auto list = entity_manager->GetChunkListsWithComponents<components::MeshRenderer, components::Transform>();
    update_renderer_system->ProcessChunks(list);
}

