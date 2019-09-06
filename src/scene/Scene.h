//
// Created by Sidorenko Nikita on 3/28/18.
//

#ifndef CPPWRAPPER_SCENE_H
#define CPPWRAPPER_SCENE_H

#include <vector>
#include <unordered_map>
#include "EngTypes.h"
#include "GameObject.h"
#include "render/renderer/ICameraParamsProvider.h"

class Scene : public IGameObjectManager {
public:

  struct Visibility {
    std::vector<GameObjectPtr> objects;
    std::vector<ProjectorPtr> projectors;
    std::vector<LightObjectPtr> lights;
    bool hasData = false; // when false, cache values are recalculated
  };

public:
  void setAsDefault();
  Scene();

  const std::unordered_map<GameObjectID, GameObjectPtr> *const gameObjectMap() const { return &_objectMap; };
  const std::vector<GameObjectPtr> *const gameObjects() const { return &_gameObjects; }

  const std::vector<ProjectorPtr> &visibleProjectors(const std::shared_ptr<ICameraParamsProvider> &camera) const {
    if (!_visibilityMap[camera].hasData) { return _getVisibilityForCamera(camera).projectors; }
    return _visibilityMap.at(camera).projectors;
  }
  const std::vector<GameObjectPtr> &visibleObjects(const std::shared_ptr<ICameraParamsProvider> &camera) const {
    if (!_visibilityMap[camera].hasData) { return _getVisibilityForCamera(camera).objects; }
    return _visibilityMap.at(camera).objects;
  }
  const std::vector<LightObjectPtr> &visibleLights(const std::shared_ptr<ICameraParamsProvider> &camera) const {
    if (!_visibilityMap[camera].hasData) { return _getVisibilityForCamera(camera).lights; }
    return _visibilityMap.at(camera).lights;
  }
  void update(float dt);

  int cameraCount() { return (int)_cameras.size(); }
  const auto &cameras() const { return _cameras; }

protected:
  Scene::Visibility &_getVisibilityForCamera(const std::shared_ptr<ICameraParamsProvider> &camera) const;

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

  std::vector<CameraPtr> _cameras; // maps GameObject::id() to Camera
  std::vector<ProjectorPtr> _projectors; // Array of projectors
  std::vector<LightObjectPtr> _lights; // Array of scene lights
  std::vector<GameObjectPtr> _gameObjects; // Full list of scene game objects
  std::unordered_map<GameObjectID, TransformPtr>_rootTransformMap; // maps GameObject::id() to the top level transforms
  mutable std::unordered_map<std::shared_ptr<ICameraParamsProvider>, Scene::Visibility> _visibilityMap; // maps camera to a visible object list
  std::vector<GameObjectPtr> _startList;

  void _processAddedObject(GameObjectPtr object);
  void _processRemovedObject(GameObjectPtr object);
};

#endif //CPPWRAPPER_SCENE_H
