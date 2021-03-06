//
// Created by Sidorenko Nikita on 4/3/18.
//

#ifndef CPPWRAPPER_CAMERA_H
#define CPPWRAPPER_CAMERA_H

#include "CommonIncludes.h"
#include "render/renderer/ICameraParamsProvider.h"
#include "utils/Frustum.h"
#include "ecs/components/Transform.h"

class Camera : public ICameraParamsProvider {
public:
  enum class Mode : int {
    Perspective = 0,
    Ortho,
    UI
  };

  const mat4 &projectionMatrix() const { return projection_matrix; }
  const mat4 &viewMatrix() const { return view_matrix; }
  const mat4 ViewProjectionMatrix() const { return view_projection_matrix; }
  const vec4 Viewport() const { return viewport; }
  const uvec2 ScreenSize() const { return uvec2(viewport.z, viewport.w); }
  void Update();

  void SetMode(Mode value) { mode = value; }
  Mode GetMode() const { return mode; }

  void orthographicSize(float orthographicSize) { orthographic_size = orthographicSize; }
  float orthographicSize() const { return orthographic_size; }

  void Fov(float fov) { this->fov = fov; }
  float Fov() const { return fov; }

  vec3 cameraForward() const { return transform.Forward(); }
  vec3 cameraLeft() const { return transform.Left(); }
  vec3 cameraRight() const{ return transform.Right(); }
  vec3 cameraUp() const { return transform.Up(); }
  vec3 cameraDown() const { return transform.Down(); }
  uvec2 cameraViewSize() const { return  ScreenSize(); }
  vec4 cameraViewport() const { return viewport; }
  unsigned int cameraIndex() const { return camera_index; }; // index is an offset in the corresponding UBO
  void cameraIndex(uint32_t index) { camera_index = index; };

  // Visibility check
  bool aabbVisible(const AABB &aabb) const { return frustum.isVisible(aabb.min, aabb.max); };
  bool obbVisible(const OBB &obb) const { return frustum.isVisible(obb.matrix, obb.min, obb.max); };
  bool sphereVisible(const vec3 &center, const float radius) const { return frustum.isVisible(center, radius); };
  void cameraVisibilityMask(unsigned int mask) { visibility_mask = mask; };

  // ICameraParamsProvider
  vec3 cameraPosition() const override { return transform.WorldPosition(); }
  mat4 cameraViewProjectionMatrix() const override { return view_projection_matrix; }
  mat4 cameraViewMatrix() const override { return view_matrix; }
  mat4 cameraProjectionMatrix() const override { return projectionMatrix(); }
  vec2 cameraZMinMax() const override { return vec2(zMin, zMax); }
  unsigned int cameraVisibilityMask() const override { return visibility_mask; };
  const Frustum &GetFrustum() const override { return frustum; };

  ECS::components::Transform& Transform() { return transform; }
  std::pair<vec3, vec3> GetMouseRay(vec2 mouse_position);

protected:
  uint32_t visibility_mask = ~0u; // all visible by default
  mat4 projection_matrix;
  mat4 view_matrix;
  vec4 viewport;
  mat4 view_projection_matrix;
  Frustum frustum;
  float zMin = 0.1;
  float zMax = 1000;
  ECS::components::Transform transform;

  float fov = 45.0f;

  Mode mode = Mode::Perspective;
  float orthographic_size = 10.0f;

  unsigned int camera_index = 0;

protected:
  void UpdateProjection();
  void UpdateView();
  void UpdateViewport();
};

typedef std::shared_ptr<Camera> CameraPtr;


#endif //CPPWRAPPER_CAMERA_H
