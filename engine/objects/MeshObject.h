#pragma once

#include "CommonIncludes.h"
#include <scene/GameObject.h>
#include <render/mesh/Mesh.h>
#include "render/material/Material.h"

class Material;

class MeshObject : public GameObject {
public:
  MeshObject();

  std::shared_ptr<Material> material() { return _material; }
  void material(std::shared_ptr<Material> material) { _material = material; }

  Mesh::Handle mesh() const { return _mesh; }
  void mesh(Mesh::Handle mesh) { _mesh = mesh; }

  void start() override;
protected:
    Mesh::Handle _mesh;
    std::shared_ptr<Material> _material;
    RenderQueue _renderQueue = RenderQueue::Opaque;

};

typedef std::shared_ptr<MeshObject> MeshObjectPtr;
