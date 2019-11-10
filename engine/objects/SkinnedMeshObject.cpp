//
// Created by Sidorenko Nikita on 2018-12-13.
//

#include "SkinnedMeshObject.h"
#include "loader/HierarchyLoader.h"
#include <iostream>
#include "resources/ModelBundle.h"
#include "scene/GameObject.h"

void SkinnedMeshObject::setSkinningData(ModelBundlePtr bundle, SkinningDataPtr skinningData) {
  if (_rootJoint) {
    DestroyGameObject(_rootJoint);
    _rootJoint = nullptr;
  } 

  _skinningData = skinningData;

  _jointList.clear();
  _jointMap.clear();
  
  if (_skinningData) {
	  
    _rootJoint = loader::loadHierarchy(bundle, _skinningData->joints);
    _rootJoint->transform()->parent(transform());

	
    if (_rootJoint->animation()->hasAnimation()) {
      _animation = _rootJoint->animation();
    }

	
    auto processJoint = [&](TransformPtr transform) {
      if (!_animation->hasAnimation() && transform->gameObject()->animation()->hasAnimation()) {
        _animation = transform->gameObject()->animation();
      }

      _jointMap[transform->gameObject()->sid()] = transform->gameObject();
      transform->gameObject()->animation()->autoUpdate(false);
      _rootJoint->animation()->addChildController(transform->gameObject()->animation());
    };

    _animation->autoUpdate(false);
    _jointMap[_rootJoint->name()] = _rootJoint;
    _rootJoint->transform()->forEachChild(true, processJoint);

    for (auto &jointName : _skinningData->jointNames) {
      _jointList.push_back(_jointMap.at(jointName));
    }

  } else {
    _animation = std::make_shared<AnimationController>();
  }
}

void SkinnedMeshObject::_processAnimations(float dt) {
  _animation->update(dt);
}

void SkinnedMeshObject::start() {
  animation()->play("default", true);
}

void SkinnedMeshObject::_debugDraw() {
 /* for (int i = 0; i < _mesh->vertexCount(); i++) {
    vec3 vertex = _mesh->getVertex(i);
    vec4 weight = _mesh->getWeights(i);
    vec4 jointIndices = _mesh->getJointIndices(i);

    mat4 matrix = _skinningMatrices.matrices[(int)jointIndices.x] * weight.x +
                  _skinningMatrices.matrices[(int)jointIndices.y] * weight.y +
                  _skinningMatrices.matrices[(int)jointIndices.z] * weight.z;
    vertex = vec3(matrix * vec4(vertex, 1));
    //getEngine()->debugDraw()->drawPoint(vertex, vec3(1, 1, 1));
  }


  transform()->forEachChild(true, [&](TransformPtr transform) {
    if (transform->parent()->gameObject()->id() == this->id()) { return; }
    //getEngine()->debugDraw()->drawLine(transform->worldPosition(), transform->parent()->worldPosition(), vec4(1, 0, 0, 1));
  }); */
};

void SkinnedMeshObject::postUpdate() {
  for (int i = 0; i < _jointList.size(); i++) {
    _skinningMatrices.matrices[i] = _jointList[i]->transform()->worldMatrix() * _skinningData->bindPoses[i];
  }
}

void SkinnedMeshObject::render(std::function<void(core::Device::RenderOperation& rop, RenderQueue queue)> callback) {
  if (!_mesh || !_material) {
    return;
  }

  RenderOperation rop = _getDefaultRenderOp();
  rop.skinning_matrices= &_skinningMatrices;
  rop.mesh = _mesh;
  rop.material = _material;
  callback(rop, _renderQueue);
}