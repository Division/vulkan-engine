#include "FollowCamera.h"
#include "PlayerController.h"
#include "Engine.h"
#include "system/Input.h"
#include "scene/Scene.h"

using namespace core;
using namespace core::system;

void FollowCamera::start() {
  auto container = CreateGameObject<GameObject>();

  container->transform()->rotate(vec3(0, 1, 0), -RAD(225));
  container->transform()->rotate(vec3(1, 0, 0), RAD(55));

  _container = container;
  setFreeCamera(false);
}


void FollowCamera::setPlayer(std::shared_ptr<PlayerController> player) {
  _player = player;
}

void FollowCamera::update(float dt) {
  auto player = _player.lock();

  if (_isFreeCamera || _player.lock() == nullptr) {
    auto input = Engine::Get()->GetInput();
    vec3 posDelta = vec3(0, 0, 0);
	if (input->keyDown(Key::E)) {
		posDelta += transform()->up();
	}

	if (input->keyDown(Key::Q)) {
		posDelta += transform()->down();
	}

    if (input->keyDown(Key::A)) {
      posDelta += transform()->left();
    }

    if (input->keyDown(Key::D)) {
      posDelta += transform()->right();
    }

    if (input->keyDown(Key::W)) {
      posDelta += transform()->forward();
    }

    if (input->keyDown(Key::S)) {
      posDelta += transform()->backward();
    }

    if (input->keyDown(Key::MouseLeft)) {
      _angleX += input->mouseDelta().y * 0.004f;
      _angleY += input->mouseDelta().x * 0.004f;
    }

    transform()->translate(posDelta * dt * 20.0f);
    quat rotation(vec3(_angleX, _angleY, 0));
    transform()->rotation(rotation);
  } else if (player) {
    _container.lock()->transform()->position(player->transform()->position() + vec3(0, 1, 0));
  }

  auto* scene_camera = Engine::Get()->GetScene()->GetCamera();
  scene_camera->transform()->setMatrix(transform()->worldMatrix());
}

void FollowCamera::setFreeCamera(bool isFree) {
  _isFreeCamera = isFree;
  if (isFree) {
    vec3 position = transform()->worldPosition();
    transform()->parent(nullptr);
    //transform()->position(position);
   // _angleX = -(float)M_PI / 8;
    //_angleY = (float)M_PI;
  } else {
    transform()->parent(_container.lock()->transform());
    transform()->position(vec3(0, 0, -15));
    transform()->rotation(quat());
  }
}
