//
// Created by Sidorenko Nikita on 4/19/18.
//

#include "MeshGeneration.h"
#include <vector>
#include <algorithm>

void MeshGeneration::generateSphere(MeshPtr mesh, int parallelCount, int meridianCount, float radius) {
  std::vector<vec3> vertices;
  std::vector<uint16_t> indices;

  for (int j = 0; j < parallelCount; j++) {
    float parallel = (float)M_PI * (float)j / (float)(parallelCount - 1);

    for (int i = 0; i < meridianCount; i++) {
      float meridian = (float)(2.0 * M_PI * (float)i / (float)meridianCount);

      vertices.emplace_back(vec3(radius * sinf(parallel) * cosf(meridian),
                         radius * sinf(parallel) * sinf(meridian),
                         radius * cosf(parallel)));

      indices.push_back(j * parallelCount + i);
      indices.push_back(((j + 1) % parallelCount) * parallelCount + i);
      indices.push_back(((j + 1) % parallelCount) * parallelCount + (i + 1) % meridianCount);

      indices.push_back(((j + 1) % parallelCount) * parallelCount + (i + 1) % meridianCount);
      indices.push_back(((j) % parallelCount) * parallelCount + (i + 1) % meridianCount);
      indices.push_back(j * parallelCount + i);
    }
  }

  mesh->setVertices(vertices);
  mesh->setIndices(indices);
}

void MeshGeneration::generateBox(MeshPtr mesh, float sizeX, float sizeY, float sizeZ) {
  auto halfX = sizeX * 0.5f;
  auto halfY = sizeY * 0.5f;
  auto halfZ = sizeZ * 0.5f;

  float srcVertices[] = {
    -halfX, -halfY, -halfZ,
    halfX, -halfY, -halfZ,
    halfX, -halfY, halfZ,
    -halfX, -halfY, halfZ,
    -halfX, halfY, -halfZ,
    halfX, halfY, -halfZ,
    halfX, halfY, halfZ,
    -halfX, halfY, halfZ,
  };

  int srcIndices[] = {
    3, 0, 2, 0, 1, 2, // +y
    4, 7, 6, 4, 6, 5, // -y
    7, 4, 0, 7, 0, 3, // -x
    5, 2, 1, 5, 6, 2, // +x
    4, 5, 1, 4, 1, 0, // -z
    7, 3, 6, 6, 3, 2  // +z
  };

  std::vector<vec3> vertices;

  // duplicate vertices
  for (int i = 0; i < 36; i++) {
    vertices.emplace_back(vec3(srcVertices[srcIndices[i] * 3],
                            srcVertices[srcIndices[i] * 3 + 1],
                            srcVertices[srcIndices[i] * 3 + 2]));
  }

  mesh->setVertices(vertices);
}

void MeshGeneration::generateCone(MeshPtr mesh, float height, float radius, int segments) {
  vec3 origin = vec3(0,0,0);
  vec3 baseCenter = vec3(0, 0, -height);

  std::vector<vec3> vertices;
  for (int i = 0; i < segments; i++) {
    float angle = (float)i / (segments - 1) * (float)M_PI * 2.0f;
    float nextAngle = (float)((i + 1) % segments) / (float)(segments - 1) * (float)M_PI * 2.0f;
    vec3 point(cosf(angle) * radius, sinf(angle) * radius, -height);
    vec3 nextPoint(cosf(nextAngle) * radius, sinf(nextAngle) * radius, -height);
    vertices.push_back(origin);
    vertices.push_back(point);
    vertices.push_back(nextPoint);

    vertices.push_back(point);
    vertices.push_back(baseCenter);
    vertices.push_back(nextPoint);
  }

  mesh->setVertices(vertices);
}

void MeshGeneration::generateFullScreenQuad(MeshPtr mesh) {
  generateQuad(mesh, vec2(2, 2));
}

void MeshGeneration::generateQuad(MeshPtr mesh, vec2 size, vec2 origin) {
  auto halfSize = vec3(size / 2.0f, 0);
  auto offset = vec3((vec2(0.5f, 0.5f) - origin) * size, 0);

  std::vector<vec3> vertices = {
      vec3(-1, 1, 0) * halfSize + offset,
      vec3(-1, -1, 0) * halfSize + offset,
      vec3(1, -1, 0) * halfSize + offset,
      vec3(1, -1, 0) * halfSize + offset,
      vec3(1, 1, 0) * halfSize + offset,
      vec3(-1, 1, 0) * halfSize + offset
  };


  std::vector<vec2> texCoords = {
      vec2(0, 1),
      vec2(0, 0),
      vec2(1, 0),
      vec2(1, 0),
      vec2(1, 1),
      vec2(0, 1)
  };

  std::reverse(vertices.begin(), vertices.end());
  std::reverse(texCoords.begin(), texCoords.end());
  mesh->setVertices(vertices);
  mesh->setTexCoord0(texCoords);
}
