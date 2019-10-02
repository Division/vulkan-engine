//
// Created by Sidorenko Nikita on 3/30/18.
//

#include "Sprite.h"
#include "render/material/Material.h"

Sprite::Sprite() : MeshObject() {
  uint16_t indices[] = {0, 1, 2, 0, 2, 3};
  float vertices[] = {    -1.0f, -1.0f, 0.0f,
                          1.0f, -1.0f, 0.0f,
                          1.0f, 1.0f, 0.0f,
                          -1.0f,  1.0f, 0.0f };

  float texcoords[] = {   0, 0,
                          1.0f, 0,
                          1.0f, 1.0f,
                          0,  1.0f };

  _mesh = std::make_shared<Mesh>();
  _mesh->setVertices(vertices, 4);
  _mesh->setIndices(indices, 6);
  _mesh->setTexCoord0(texcoords, 4);
  _mesh->createBuffer();
}
