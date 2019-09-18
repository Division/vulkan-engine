#pragma once

#include "CommonIncludes.h"
#include "render/mesh/Mesh.h"

namespace MeshGeneration {
	
  void generateSphere(MeshPtr mesh, int parallelCount, int meridianCount, float radius = 1.0f);
  void generateBox(MeshPtr mesh, float sizeX, float sizeY, float sizeZ);
  void generateCone(MeshPtr mesh, float height, float radius, int segments = 12);
  void generateQuad(MeshPtr mesh, vec2 size, vec2 origin = vec2(0.5, 0.5));
  void generateFullScreenQuad(MeshPtr mesh);
  
};

