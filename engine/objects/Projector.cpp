//
//
// Created by Sidorenko Nikita on 11/15/18.
//

#include "Projector.h"
#include "render/shader/ShaderBufferStruct.h"
#include "render/renderer/SceneRenderer.h"

void Projector::postUpdate() {
  _viewMatrix = glm::inverse(transform()->worldMatrix());
  _viewProjection = _getProjection() * _viewMatrix;
  _frustum.calcPlanes(_viewProjection);

  auto position = transform()->worldPosition();

  if (_isOrthographic) {
    float height = _orthographicSize;
    float width = _aspect * _orthographicSize;
    _cullingData.sphere.position = transform()->forward() * ((_zFar - _zNear) * 0.5f + _zNear) + position;
    _cullingData.sphere.radius = glm::length(vec3(width / 2, height / 2, (_zFar - _zNear) / 2));
  } else {
    _cullingData.sphere = boundingSphereForFrustum(_aspect, 1, _zNear, _zFar, RAD(_fov));
    _cullingData.sphere.position = transform()->backward() * _cullingData.sphere.position.z + position;
  }

  _cullingData.type = CullingData::Type::Sphere;

  if (_debugEnabled) {
    //auto debugDraw = getEngine()->debugDraw();
    //debugDraw->drawFrustum(_viewProjection, glm::vec4(1,1,1,1));
  }
}

mat4 Projector::_getProjection() const {
  if (_isOrthographic) {
    float halfHeight = _orthographicSize / 2.0f;
    float halfWidth = _aspect * halfHeight;
    return glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, _zNear, _zFar);
  } else {
    return glm::perspective(RAD(_fov), _aspect, _zNear, _zFar);
  }
}

ShaderBufferStruct::Projector Projector::getProjectorStruct() const {
	ShaderBufferStruct::Projector result;

  result.position = transform()->worldPosition();
  result.attenuation = _squareAttenuation;
  result.radius = _zFar;
  result.color = _color;
  result.scale = vec2(_spriteBounds.width, _spriteBounds.height);
  result.offset = vec2(_spriteBounds.x, _spriteBounds.y);
  result.mask = cameraVisibilityMask();

  if (castShadows()) {
	  result.shadowmapScale = vec2(_viewport.z, _viewport.w) / (float)core::render::SceneRenderer::ShadowAtlasSize();
	  result.shadowmapOffset= vec2(_viewport.x, _viewport.y) / (float)core::render::SceneRenderer::ShadowAtlasSize();
  } else {
	  result.shadowmapScale = vec2(0, 0);
  }

  result.projectionMatrix = _viewProjection;

  return result;
}

// Fills the result vector with world-space frustum edge vertices
void Projector::getEdgePoints (std::vector<vec3> &outEdgePoints) {
  outEdgePoints.resize(8);

  auto inv = glm::inverse(_viewProjection);
  vec4 quad1[4] = {
      inv * vec4(1, -1, -1, 1),
      inv * vec4(-1, -1, -1, 1),
      inv * vec4(-1, 1, -1, 1),
      inv * vec4(1, 1, -1, 1),
  };

  vec4 quad2[4] = {
      inv * vec4(1, -1, 1, 1),
      inv * vec4(-1, -1, 1, 1),
      inv * vec4(-1, 1, 1, 1),
      inv * vec4(1, 1, 1, 1),
  };

  for (int i = 0; i < 8; i++) {
    auto &quad = i < 4 ? quad1 : quad2;
    int index = i % 4;
    outEdgePoints[i] = vec3(quad[index] / quad[index].w);
  }
}

void Projector::attenuation(float linear, float square) {
  _linearAttenuation = linear;
  _squareAttenuation = square;
}
