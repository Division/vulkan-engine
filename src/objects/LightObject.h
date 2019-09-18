//
// Created by Sidorenko Nikita on 4/18/18.
//

#ifndef CPPWRAPPER_LIGHTOBJECT_H
#define CPPWRAPPER_LIGHTOBJECT_H

#include "scene/GameObject.h"
#include <memory>
#include "render/shader/ShaderBufferStruct.h"
#include "utils/MeshGeneration.h"
#include "render/shading/IShadowCaster.h"

enum class LightObjectType : int {
  Point = 0,
  Spot
};

class Texture;

class LightObject : public GameObject, public IShadowCaster {
public:
  friend class Scene;
  friend class ShadowMap;

  LightObject();

  float radius() const { return _radius; }
  void radius(float value) { _radius = value; }

  float attenuation() const { return _attenuation; }
  void attenuation(float value) { _attenuation = value; }

  LightObjectType type() const { return _type; }
  void type(LightObjectType value) { _type = value; }

  vec3 color() const { return _color; }
  void color(vec3 value) { _color = value; }

  float coneAngle() const { return _coneAngle; }
  void coneAngle(float value) { _coneAngle = value; }

  void setFlare(const std::shared_ptr<Texture>& texture, float size);

  float getSpotRadius(float height);

  bool castShadows() const override { return _castShadows; };
  void castShadows(bool value) { _castShadows = value; }
  void cameraVisibilityMask(unsigned int mask) { _visibilityMask = mask; };

  uint32_t index() const { return _index; }
  void index(uint32_t index) { _index = index; }

  void enableDebug();
  //bool debugEnabled() { return _debugMesh && _debugMaterial; }

  ShaderBufferStruct::Light getLightStruct() const;

  void render(IRenderer &renderer) override;
  void postUpdate() override;

  // ICameraParamsProvider
  uvec2 cameraViewSize() const override { return  uvec2(_viewport.z, _viewport.w); }
  vec3 cameraPosition() const override { return transform()->worldPosition(); }
  mat4 cameraViewProjectionMatrix() const override { return _projectionMatrix * _viewMatrix; }
  vec3 cameraLeft() const override { return transform()->left(); }
  vec3 cameraRight() const override { return transform()->right(); }
  vec3 cameraUp() const override { return transform()->up(); }
  vec3 cameraDown() const override { return transform()->down(); }
  mat4 cameraViewMatrix() const override { return _viewMatrix; }
  mat4 cameraProjectionMatrix() const override { return _projectionMatrix; }
  vec4 cameraViewport() const override { return _viewport; }
  unsigned int cameraVisibilityMask() const override { return _visibilityMask; };
  const Frustum &frustum() const override { return _frustum; };
  unsigned int cameraIndex() const override { return _cameraIndex; };
  void cameraIndex(uint32_t index) override { _cameraIndex = index; };

private:
  // ICameraParamsProvider
  uint32_t _visibilityMask = ~0u; // all visible by default

  // Common light properties
  float _radius = 13;
  vec3 _color = vec3(1, 1, 1);
  float _attenuation = 2;
  uint32_t _index;

  // Spotlight properties
  float _coneAngle = 30;

  // Flare
  //MeshPtr _flareMesh; // Right now the mesh is not shared. Will be improved after particle render implemented.
  float _flareSize = 0;
  std::shared_ptr<Texture> _flareTexture;
  //std::shared_ptr<MaterialBillboard> _flareMaterial;

  // Shadows
  Frustum _frustum;
  mat4 _projectionMatrix;
  mat4 _viewMatrix;
  vec4 _viewport;
  float _zMin = 0.1f;
  bool _castShadows = false;
  unsigned int _cameraIndex = 0;

  LightObjectType _type = LightObjectType::Point;

  //MeshPtr _debugMesh;
  //MaterialSingleColorPtr _debugMaterial;

protected:
  // To be used by ShadowMap class
  void viewport(vec4 value) override { _viewport = value; };
  vec4 viewport() const override { return _viewport; };
};

typedef std::shared_ptr<LightObject> LightObjectPtr;

#endif //CPPWRAPPER_LIGHTOBJECT_H
