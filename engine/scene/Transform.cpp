//
// Created by Sidorenko Nikita on 3/28/18.
//

#include "Transform.h"
#include "GameObject.h"

void Transform::_updateTransform(const mat4 *parentTransform, bool parentUpdated, bool skipChildren) const {

  // Update local transform if needed
  if (_dirty) {
    _localMatrix = mat4();
    _localMatrix = glm::translate(_localMatrix, _position);
    _localMatrix *= mat4_cast(_rotation);
    _localMatrix = glm::scale(_localMatrix, _scale);
  }

  if (parentUpdated || _dirty) {
    if (parentTransform) {
      _worldMatrix = *parentTransform * _localMatrix;
    } else {
      _worldMatrix = _localMatrix;
    }
  }

  if (!skipChildren) {
    for (auto &childTransform : _children) {
      childTransform->_updateTransform(&_worldMatrix, _dirty || parentUpdated);
    }
  }

  _dirty = false;
}

void Transform::_addChild(std::shared_ptr<Transform> child) {
  bool found = false;
  for (auto &transform : _children) {
    if (transform->gameObject()->id() == child->gameObject()->id()) {
      found = true;
      break;
    }
  }

  if (!found) {
    _children.push_back(child);
  }
}

void Transform::_removeChild(TransformPtr child) {
  for (int i = 0; i < _children.size(); i++) {
    if (_children[i]->gameObject()->id() == child->gameObject()->id()) {
      // Remove i-th element:
      // Replace i-th element with the last and pop array
      _children[i] = _children.back();
      _children.pop_back();
    }
  }
}

void Transform::setParent(TransformPtr transform) {
  if (this->parent() != transform) {
    _manager->transformChangeParent(shared_from_this(), this->parent(), transform);
    _dirty = true;
  }
}

void Transform::rotate(vec3 axis, float angle) {
  _rotation = glm::rotate(_rotation, angle, axis);
  setDirty();
}

const vec3 Transform::left() const {
  return -right();
}

const vec3 Transform::up() const {
  _updateTransformUpwards();
  return vec3(_worldMatrix[1]);
}

const vec3 Transform::forward() const {
  return -backward();
}

const vec3 Transform::right() const {
  _updateTransformUpwards();
  return vec3(_worldMatrix[0]);
}

const vec3 Transform::down() const {
  return -up();
}

const vec3 Transform::backward() const {
  _updateTransformUpwards();
  return vec3(_worldMatrix[2]);
}

void Transform::setMatrix(const mat4 &matrix) {
  quat r;
  vec3 skew;
  vec4 perspective;
  glm::decompose(matrix, _scale, r, _position, skew, perspective);
  _rotation = glm::conjugate(r);
  setDirty();
}

void Transform::_updateTransformUpwards(bool skipParentUpdate) const {
  auto parentPtr = parent();

  if (!skipParentUpdate && parentPtr) {
    parentPtr->_updateTransformUpwards();
  }

  if (_dirty) {
    mat4 *matrix = parentPtr ? &parentPtr->_worldMatrix : nullptr;
    _updateTransform(matrix, false, true);
  }
}

void Transform::forEachChild(bool recursive, std::function<void(std::shared_ptr<Transform>)> callback) const {
  for (auto &child : _children) {
    callback(child);
  }

  if (recursive) {
    for (auto &child : _children) {
      child->forEachChild(true, callback);
    }
  }
}
