#pragma once

#include "CommonIncludes.h"
#include "GameObject.h"
#include "render/renderer/ICameraParamsProvider.h"

class Projector;
class LightObject;
class Camera;

namespace core
{
	namespace ECS
	{
		class EntityManager;
	}
}

class Scene : public IGameObjectManager {
public:

  struct Visibility {
    std::vector<GameObjectPtr> objects;
    std::vector<std::shared_ptr<Projector>> projectors;
    std::vector<std::shared_ptr<LightObject>> lights;
    bool hasData = false; // when false, cache values are recalculated
  };

public:
  void setAsDefault();
  Scene();

  const std::unordered_map<GameObjectID, GameObjectPtr> *const gameObjectMap() const { return &_objectMap; };
  const std::vector<GameObjectPtr> *const gameObjects() const { return &_gameObjects; }

  const std::vector<std::shared_ptr<Projector>> &visibleProjectors(const ICameraParamsProvider* camera) const {
    if (!_visibilityMap[camera].hasData) { return _getVisibilityForCamera(camera).projectors; }
    return _visibilityMap.at(camera).projectors;
  }
  const std::vector<GameObjectPtr> &visibleObjects(const ICameraParamsProvider* camera) const {
    if (!_visibilityMap[camera].hasData) { return _getVisibilityForCamera(camera).objects; }
    return _visibilityMap.at(camera).objects;
  }
  const std::vector<std::shared_ptr<LightObject>> &visibleLights(const ICameraParamsProvider* camera) const {
    if (!_visibilityMap[camera].hasData) { return _getVisibilityForCamera(camera).lights; }
    return _visibilityMap.at(camera).lights;
  }
  void update(float dt);

  Camera* GetCamera() const { return camera.get(); }

  core::ECS::EntityManager* GetEntityManager() const { return entity_manager; }
  void SetEntityManager(core::ECS::EntityManager* manager) { entity_manager = manager; }

protected:
  Scene::Visibility &_getVisibilityForCamera(const ICameraParamsProvider *camera) const;

  // IGameObjectManager
  void addGameObject(GameObjectPtr object) override;
  void destroyGameObject(GameObjectPtr object) override;

  // ITransformManager
  void transformChangeParent(TransformPtr transform, TransformPtr oldParent, TransformPtr newParent) override;

  // update
  void _updateTransforms();

  // Helper
  bool _objectIsVisible(const GameObjectPtr &object, const Frustum &frustum) const {
    if (!object->active()) { return false; }
    auto &cullingData = object->cullingData();
    if (cullingData.type == CullingData::Type::Sphere) {
      return frustum.isVisible(cullingData.sphere.position, cullingData.sphere.radius);
    } else {
      return frustum.isVisible(object->transform()->worldMatrix(), cullingData.bounds.min, cullingData.bounds.max);
    }
  }

protected:
  unsigned int _lightCount = 0;

  std::unordered_map<GameObjectID, GameObjectPtr> _objectMap; // maps GameObject::id() to GameObject

  std::shared_ptr<Camera> camera;
  std::vector<std::shared_ptr<Projector>> _projectors; // Array of projectors
  std::vector<std::shared_ptr<LightObject>> _lights; // Array of scene lights
  std::vector<GameObjectPtr> _gameObjects; // Full list of scene game objects
  std::unordered_map<GameObjectID, TransformPtr>_rootTransformMap; // maps GameObject::id() to the top level transforms
  mutable std::unordered_map<const ICameraParamsProvider*, Scene::Visibility> _visibilityMap; // maps camera to a visible object list
  std::vector<GameObjectPtr> _startList;

  core::ECS::EntityManager* entity_manager = nullptr;

  void _processAddedObject(GameObjectPtr object);
  void _processRemovedObject(GameObjectPtr object);
};
