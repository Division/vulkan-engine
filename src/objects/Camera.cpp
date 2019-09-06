//
// Created by Sidorenko Nikita on 4/3/18.
//

#include "Camera.h"
#include "Engine.h"
#include "EngineMath.h"
//#include "render/renderer/Renderer.h"
#include "system/Window.h"

void Camera::_updateProjection() {
  //auto engine = getEngine();
  auto window = Engine::Get()->window();

  float width = 800.0f;
  float height = 600.0f;
  float aspect = width / height;
  switch (_mode) {
    case Mode::Perspective:
      _projectionMatrix = glm::perspective(glm::radians(_fov), window->aspect(), 0.1f, 100.0f);
      break;

    case Mode::Ortho: {
      float halfHeight = _orthographicSize / 2.0f;
      float halfWidth = aspect * halfHeight;
      _projectionMatrix = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, 0.1f, 100.0f);
      break;
    }
    case Mode::UI:
      float halfHeight = height / 2.0f;
      float halfWidth = width / 2.0f;
      _projectionMatrix = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, -1.0f, 1.0f);
      break;
  }
}

void Camera::_updateView() {
  _viewMatrix = glm::inverse(transform()->worldMatrix());
}

void Camera::_updateViewport() {
	auto window = Engine::Get()->window();
  _viewport = window->viewport(); // TODO: modify if camera rendertarget support is required
}

void Camera::postUpdate() {
  this->_updateViewport();
  this->_updateProjection();
  this->_updateView();
  _viewProjectionMatrix = _projectionMatrix * _viewMatrix;
  _frustum.calcPlanes(_viewProjectionMatrix);
}
