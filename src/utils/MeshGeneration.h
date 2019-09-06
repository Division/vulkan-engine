//
// Created by Sidorenko Nikita on 4/19/18.
//

#ifndef CPPWRAPPER_MESHGENERATION_H
#define CPPWRAPPER_MESHGENERATION_H

#include "EngineMath.h"
#include <vector>
#include "render/mesh/Mesh.h"

namespace MeshGeneration {
	
  void generateSphere(MeshPtr mesh, int parallelCount, int meridianCount, float radius = 1.0f);
  void generateBox(MeshPtr mesh, float sizeX, float sizeY, float sizeZ);
  void generateCone(MeshPtr mesh, float height, float radius, int segments = 12);
  void generateQuad(MeshPtr mesh, vec2 size, vec2 origin = vec2(0.5, 0.5));
  void generateFullScreenQuad(MeshPtr mesh);
  
};


#endif //CPPWRAPPER_MESHGENERATION_H
