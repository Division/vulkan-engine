//
// Created by Sidorenko Nikita on 3/30/18.
//

#ifndef CPPWRAPPER_MESHOBJECT_H
#define CPPWRAPPER_MESHOBJECT_H

#include <scene/GameObject.h>
#include <render/mesh/Mesh.h>
#include "render/material/Material.h"
#include <memory>

class MeshObject : public GameObject {
public:
  MeshObject();

  MaterialPtr material() { return _material; }
  void material(MaterialPtr material) { _material = material; }

  MeshPtr mesh() const { return _mesh; }
  void mesh(MeshPtr mesh) { _mesh = mesh; }

  void start() override;
protected:
  MeshPtr _mesh;
  MaterialPtr _material;
  RenderQueue _renderQueue = RenderQueue::Opaque;
  void render(IRenderer &renderer) override;

};

typedef std::shared_ptr<MeshObject> MeshObjectPtr;


#endif //CPPWRAPPER_MESHOBJECT_H
