//
// Created by Sidorenko Nikita on 4/18/18.
//

#include "LightObject.h"
#include <memory>
//#include "EngineMain.h"
//#include "render/debug/DebugDraw.h"
#include "render/renderer/SceneRenderer.h"
#include "utils/Math.h"
#include "utils/MeshGeneration.h"

LightObject::LightObject() : GameObject() {
  _cullingData.type = CullingData::Type::Sphere;
  _layer = ~0u;
}


ConstantBufferStruct::Light LightObject::getLightStruct() const {
	ConstantBufferStruct::Light result;

  result.position = transform()->worldPosition();
  result.attenuation = attenuation();
  result.radius = radius();
  result.color = color();
  result.mask = cameraVisibilityMask();

  // Assign for point light as well because point light shadow is calculated like spotlight shadow
  result.coneAngle = cosf(RAD(_coneAngle) / 2.0f);
  result.direction = glm::normalize(transform()->forward());

  if (castShadows()) {
    result.shadowmapScale = vec2(_viewport.z, _viewport.w) / (float)SceneRenderer::shadowAtlasSize();
    result.shadowmapOffset= vec2(_viewport.x, _viewport.y) / (float)SceneRenderer::shadowAtlasSize();
    result.projectionMatrix = cameraViewProjectionMatrix();
  } else {
    result.shadowmapScale = vec2(0, 0);
  }

  return result;
}


void LightObject::enableDebug() {
  /*if (_debugMesh) {
    return;
  }

  
  _debugMesh = std::make_shared<Mesh>();
  switch (_type) {
    case LightObjectType::Point:
      MeshGeneration::generateSphere(_debugMesh, 10, 10, _radius);
      break;

    case LightObjectType::Spot:
      float spotRadius = getSpotRadius(1);
      MeshGeneration::generateCone(_debugMesh, 1, spotRadius);
  }
  _debugMesh->createBuffer();

  _debugMaterial = std::make_shared<MaterialSingleColor>(); */
}

void LightObject::render(IRenderer &renderer) {
	/*
  if (_flareTexture) {
    RenderOperation rop = _getDefaultRenderOp();
    rop.mesh = _flareMesh;
    rop.material = _flareMaterial;
    _flareMaterial->color(vec4(color(), 1));
    rop.depthTest = false;
    renderer.addRenderOperation(rop, RenderQueue::Translucent);
  }

  if (_debugMesh && _debugMaterial) {
    _debugMaterial->color(vec4(color(), 1));
    RenderOperation rop = _getDefaultRenderOp();
    rop.mesh = _debugMesh;
    rop.material = _debugMaterial;
    renderer.addRenderOperation(rop, RenderQueue::Opaque);
  } */
}

float LightObject::getSpotRadius(float height) {
  if (_type == LightObjectType::Spot) {
    return tanf(_coneAngle / 2.0f * (float)M_PI / 180) * height;
  }

  return 0;
}

void LightObject::postUpdate() {
  // TODO: add directional support
  // Shadow maps are square, so aspect is 1
  _projectionMatrix = glm::perspective(glm::radians(_coneAngle), 1.0f, _zMin, _radius);
  _viewMatrix = glm::inverse(transform()->worldMatrix());
  _frustum.calcPlanes(_projectionMatrix * _viewMatrix);

  auto position = transform()->worldPosition();
  if (_type == LightObjectType::Spot) {
    _cullingData.sphere = boundingSphereForFrustum(1, 1, 0, _radius, RAD(_coneAngle));
    _cullingData.sphere.position += position;
  } else {
    _cullingData.sphere.position = position;
    _cullingData.sphere.radius = _radius;
  };
}

void LightObject::setFlare(const TexturePtr &texture, float size) {
  _flareTexture = texture;

  /*

  if (fabs(size - _flareSize) > 0.0001 && size > 0) {
    _flareSize = size;
    if (!_flareMesh) {
      _flareMesh = std::make_shared<Mesh>(true, 3, GL_DYNAMIC_DRAW);
      _flareMaterial = std::make_shared<MaterialBillboard>();

      float halfSize = size / 2.0f;

      std::vector<vec2> corners = {
          vec2(-1, 1) * halfSize,
          vec2(-1, -1) * halfSize,
          vec2(1, -1) * halfSize,
          vec2(1, -1) * halfSize,
          vec2(1, 1) * halfSize,
          vec2(-1, 1) * halfSize
      };

      _flareMesh->setCorners(corners);
    }

    MeshGeneration::generateQuad(_flareMesh, vec2(0));
    _flareMesh->createBuffer();
  }

  if (_flareSize <= 0.0001) {
    _flareTexture = nullptr;
  }

  _flareMaterial->texture(_flareTexture);
  _isRenderable = (bool)_flareTexture;

  */
}
