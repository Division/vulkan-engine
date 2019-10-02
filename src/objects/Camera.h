//
// Created by Sidorenko Nikita on 4/3/18.
//

#ifndef CPPWRAPPER_CAMERA_H
#define CPPWRAPPER_CAMERA_H

#include "CommonIncludes.h"
#include "scene/GameObject.h"
#include "render/renderer/ICameraParamsProvider.h"
#include "utils/Frustum.h"

class Camera : public GameObject, public ICameraParamsProvider {
public:
  enum class Mode : int {
    Perspective = 0,
    Ortho,
    UI
  };

  const mat4 &projectionMatrix() const { return _projectionMatrix; }
  const mat4 &viewMatrix() const { return _viewMatrix; }
  const mat4 viewProjectionMatrix() const { return _viewProjectionMatrix; }
  const vec4 viewport() const { return _viewport; }
  const uvec2 screenSize() const { return uvec2(_viewport.z, _viewport.w); }
  void postUpdate() override;

  void mode(Mode value) { _mode = value; }
  Mode mode() const { return _mode; }

  void orthographicSize(float orthographicSize) { _orthographicSize = orthographicSize; }
  float orthographicSize() const { return _orthographicSize; }

  void fov(float fov) { _fov = fov; }
  float fov() const { return _fov; }

  // Visibility check
  bool aabbVisible(const AABB &aabb) const { return _frustum.isVisible(aabb.min, aabb.max); };
  bool obbVisible(const OBB &obb) const { return _frustum.isVisible(obb.matrix, obb.min, obb.max); };
  bool sphereVisible(const vec3 &center, const float radius) const { return _frustum.isVisible(center, radius); };
  void cameraVisibilityMask(unsigned int mask) { _visibilityMask = mask; };

  // ICameraParamsProvider
  uvec2 cameraViewSize() const override { return  screenSize(); }
  vec3 cameraPosition() const override { return transform()->worldPosition(); }
  mat4 cameraViewProjectionMatrix() const override { return viewProjectionMatrix(); }
  vec3 cameraLeft() const override { return transform()->left(); }
  vec3 cameraRight() const override { return transform()->right(); }
  vec3 cameraUp() const override { return transform()->up(); }
  vec3 cameraDown() const override { return transform()->down(); }
  mat4 cameraViewMatrix() const override { return viewMatrix(); }
  mat4 cameraProjectionMatrix() const override { return projectionMatrix(); }
  vec4 cameraViewport() const override { return viewport(); }
  unsigned int cameraVisibilityMask() const override { return _visibilityMask; };
  const Frustum &frustum() const override { return _frustum; };
  unsigned int cameraIndex() const override { return _cameraIndex; }; // index is an offset in the corresponding UBO
  void cameraIndex(uint32_t index) override { _cameraIndex = index; };

protected:
  unsigned int _visibilityMask = ~0u; // all visible by default
  mat4 _projectionMatrix;
  mat4 _viewMatrix;
  vec4 _viewport;
  mat4 _viewProjectionMatrix;
  Frustum _frustum;

  float _fov = 45.0f;

  Mode _mode = Mode::Perspective;
  float _orthographicSize = 0.0f;

  unsigned int _cameraIndex = 0;
protected:
  inline void _updateProjection();
  inline void _updateView();

  void _updateViewport();
};

typedef std::shared_ptr<Camera> CameraPtr;


#endif //CPPWRAPPER_CAMERA_H
