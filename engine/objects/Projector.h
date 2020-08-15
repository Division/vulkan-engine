#pragma once

#include "CommonIncludes.h"
#include "scene/GameObject.h"
#include "utils/MeshGeneration.h"
#include "render/shading/IShadowCaster.h"
#include "render/shader/ShaderBufferStruct.h"

enum class ProjectorType : int {
  Projector = 0, // Calculates as a light which color is based on the texture
  Decal // substitutes diffuse color and in some cases normals of the rendered object
};

// Object to use for decals and projective light sources rendering
class Projector: public GameObject, public IShadowCaster {
public:
  friend class Scene;

  Projector() = default;

  vec4 color() const { return _color; }
  void color(vec4 value) { _color = value; }

  // counts as fovY
  void fov(float fov) { _fov = fov; }
  float fov() const { return _fov; }

  void zNear(float zNear) { _zNear = zNear; }
  float zNear() const { return _zNear; }

  void zFar(float zFar) { _zFar = zFar; }
  float zFar() const { return _zFar; }

  void isOrthographic(bool isOrthographic) { _isOrthographic = isOrthographic; }
  bool isOrthographic() const { return _isOrthographic; }

  void orthographicSize(float orthographicSize) { _orthographicSize = orthographicSize; }
  float orthographicSize() const { return _orthographicSize; }

  void aspect(float aspect) { _aspect = aspect; }
  float aspect() const { return _aspect; }

  float linearAttenuation() const { return _linearAttenuation; }
  void linearAttenuation(float value) { _linearAttenuation = value; }

  float squareAttenuation() const { return _squareAttenuation; }
  void squareAttenuation(float value) { _squareAttenuation = value; }

  void attenuation(float linear, float square);

  Rect spriteBounds() const { return _spriteBounds; }
  void spriteBounds(Rect value) { _spriteBounds = value; }

  // Frustum matrix
  const mat4 viewProjection() const { return _viewProjection; }

  void getEdgePoints (std::vector<vec3> &outEdgePoints);

  ProjectorType type() const { return _type; }
  void type(ProjectorType value) { _type = value; }

  unsigned int index() const { return _index; }
  void index(unsigned int index) { _index = index; }
  void cameraVisibilityMask(unsigned int mask) { _visibilityMask = mask; };

  void setDebugEnabled(bool enabled) { _debugEnabled = enabled; }

  Device::ShaderBufferStruct::Projector getProjectorStruct() const;

  void postUpdate() override;

  // IShadowCaster
  uvec2 cameraViewSize() const override { return uvec2(viewport()); }
  vec3 cameraPosition() const override { return transform()->worldPosition(); }
  mat4 cameraViewProjectionMatrix() const override { return _viewProjection; }
  vec3 cameraLeft() const override { return transform()->left(); }
  vec3 cameraRight() const override { return transform()->right(); }
  vec3 cameraUp() const override { return transform()->up(); }
  vec3 cameraDown() const override { return transform()->down(); }
  mat4 cameraViewMatrix() const override { return _viewMatrix; }
  mat4 cameraProjectionMatrix() const override { return _getProjection(); }
  vec4 cameraViewport() const override { return viewport(); }
  unsigned int cameraVisibilityMask() const override { return _visibilityMask; };
  const Frustum &GetFrustum() const override { return _frustum; };
  unsigned int cameraIndex() const override { return _cameraIndex; }; // index is an offset in the corresponding UBO
  void cameraIndex(uint32_t index) override { _cameraIndex = index; };
  bool castShadows() const override { return this->type() == ProjectorType::Projector && _castShadows; }; // only projectors cast shadows
  void castShadows(bool value) { _castShadows = value; }

protected:
  void viewport(vec4 value) override { _viewport = value; };
  vec4 viewport() const override { return _viewport; };

private:
  // ICameraParamsProvider
  unsigned int _visibilityMask = ~0u; // all visible by default

  // common
  vec4 _color = vec4(1, 1, 1, 1);
  unsigned int _index = 0; // index in scene array
  Frustum _frustum;
  mat4 _viewProjection;
  mat4 _viewMatrix;
  float _aspect = 1;
  float _zNear = 1;
  float _zFar = 10;
  float _fov = 30; // Perspective projectors properties in degrees
  float _squareAttenuation = 0.44f;
  float _linearAttenuation = 0.35f;
  ProjectorType _type = ProjectorType::Decal;
  Rect _spriteBounds = Rect(0, 0, 1, 1);

  // Orthographic
  bool _isOrthographic = false;
  float _orthographicSize = 1.0f;

  vec4 _viewport;
  bool _castShadows = false;

  unsigned int _cameraIndex = 0;

  bool _debugEnabled = false;

private:
  mat4 _getProjection() const;
};
