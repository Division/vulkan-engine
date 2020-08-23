//
// Created by Sidorenko Nikita on 2018-12-27.
//

#include "Math.h"
#include "cmath"

using namespace glm;

Sphere boundingSphereForFrustum(float width, float height, float zNear, float zFar, float fov) {
  Sphere result;
  float k = sqrtf(1 + height * height / (width * width)) * tanf(fov / 2);
  float squareK = k * k;
  if (squareK >= (zFar - zNear) / (zFar + zNear)) {
    result.position = vec3(0, 0, -zFar);
    result.radius = zFar * k;
  } else {
    result.position = vec3(0, 0, -0.5f * (zNear + zFar) * (1.0f + squareK));
    result.radius = 0.5f * sqrtf(powf(zFar - zNear, 2) + 2.0f * (zFar * zFar + zNear * zNear) * squareK + powf(zFar + zNear, 2) * squareK * squareK);
  }

  return result;
};

const vec3 VectorLeft(-1, 0, 0);
const vec3 VectorRight(1, 0, 0);
const vec3 VectorUp(0, 1, 0);
const vec3 VectorDown(0, -1, 0);
const vec3 VectorForward(0, 0, -1);
const vec3 VectorBack(0, 0, 1);

float glm::dotf(const vec2 &a, const vec3 &b) { return a.x * b.x + a.y * b.y; }
float glm::dotf(const vec3 &a, const vec3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
float glm::dotf(const vec4 &a, const vec4 &b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }