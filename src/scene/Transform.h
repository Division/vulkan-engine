//
// Created by Sidorenko Nikita on 3/28/18.
//

#pragma once

#include "CommonIncludes.h"

class GameObject;
class Transform;

class ITransformManager {
public:
  virtual void transformChangeParent(std::shared_ptr<Transform> transform, std::shared_ptr<Transform> oldParent,
                                     std::shared_ptr<Transform> newParent) = 0;
};

class Transform : public std::enable_shared_from_this<Transform> {
private:
  explicit Transform(std::shared_ptr<GameObject> object) : _gameObject(object) {};

public:
  Transform(const Transform &t) = delete;

  template <typename T> friend std::shared_ptr<T> CreateGameObject();
  friend class GameObject;
  friend class Scene;

  std::shared_ptr<GameObject> gameObject() { return _gameObject.lock(); }
  std::shared_ptr<Transform> parent() { return _parent.lock(); }
  const std::shared_ptr<Transform> parent() const { return _parent.lock(); }

  void parent(std::shared_ptr<Transform> transform) { setParent(transform); };
  void setParent(std::shared_ptr<Transform> transform);

  const vec3 left() const;
  const vec3 right() const;
  const vec3 up() const;
  const vec3 down() const;
  const vec3 forward() const;
  const vec3 backward() const;

  const vec3 &position() const { return _position; }
  const vec3 worldPosition(bool skipUpdate = false) const { if (!skipUpdate) { _updateTransformUpwards(); }; return vec3(_worldMatrix[3]); }
  void position(const vec3 &position) { _position = position; setDirty(); }
  const quat &rotation() { return _rotation; }
  void rotation(const quat &rotation) { _rotation = rotation; setDirty(); }
  const vec3 &scale() { return _scale; }
  void scale(const vec3 &scale) { _scale = scale; setDirty(); }

  void translate(const vec3 &delta) { position(_position + delta); }
  void setMatrix(const mat4 &matrix);

  const mat4 &worldMatrix() const { return _worldMatrix; }

  void setDirty() { _dirty = true; }

  const auto children() const { return &_children; }

  std::shared_ptr<Transform> rootTransform() { if (parent()) return parent()->rootTransform(); else return shared_from_this(); }

  void forEachChild(bool recursive, std::function<void(std::shared_ptr<Transform>)> callback) const;

  void setPosition(const vec3 &position) { _position = position; setDirty(); }
  void setRotation(const quat &rotation) { _rotation = rotation; setDirty(); }
  void setScale(const vec3 &scale) { _scale = scale; setDirty(); }
  void rotate(vec3 axis, float angle);

private:
  vec3 _position = vec3(0, 0, 0);
  quat _rotation;
  vec3 _scale = vec3(1, 1, 1);
  mutable mat4 _localMatrix;
  mutable mat4 _worldMatrix;

  std::weak_ptr<GameObject> _gameObject;
  std::weak_ptr<Transform> _parent;
  std::vector<std::shared_ptr<Transform>> _children;
  ITransformManager *_manager = nullptr;

  mutable bool _dirty = true;

private:
  void _updateTransform(const mat4 *parentTransform, bool parentUpdated, bool skipChildren = false) const;
  void _updateTransformUpwards(bool skipParentUpdate = false) const;

  void _removeChild(std::shared_ptr<Transform> child);
  void _addChild(std::shared_ptr<Transform> child);
};

typedef std::shared_ptr<Transform> TransformPtr;
