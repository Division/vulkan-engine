#pragma once

#include "CommonIncludes.h"
#include "render/mesh/Mesh.h"

namespace MeshGeneration {
	
  void generateSphere(Mesh* mesh, int parallelCount, int meridianCount, float radius = 1.0f);
  void generateBox(Mesh* mesh, float sizeX, float sizeY, float sizeZ);
  void generateCone(Mesh* mesh, float height, float radius, int segments = 12);
  void generateQuad(Mesh* mesh, vec2 size, vec2 origin = vec2(0.5, 0.5));
  void generateIndexedQuad(Mesh* mesh, vec2 size, vec2 origin = vec2(0.5, 0.5));
  void generateParticleQuad(Mesh* mesh);
  void generateFullScreenQuad(Mesh* mesh);
  
};

