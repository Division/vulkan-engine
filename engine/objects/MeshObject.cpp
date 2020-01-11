//
// Created by Sidorenko Nikita on 3/30/18.
//

#include "MeshObject.h"
#include "render/material/Material.h"

MeshObject::MeshObject() : GameObject() {
  _isRenderable = true;
  _cullingData.type = CullingData::Type::OBB;
  _material = std::make_shared<Material>();
}

void MeshObject::render(std::function<void(core::Device::RenderOperation& rop, RenderQueue queue)> callback) {
  if (!_mesh || !_material) {
    return;
  }

  RenderOperation rop = _getDefaultRenderOp();
  rop.mesh = _mesh.get();
  rop.material = _material.get();
  callback(rop, _renderQueue);
}

void MeshObject::start() {
  if (_mesh) {
    _cullingData.bounds = _mesh->aabb();
  }
}
