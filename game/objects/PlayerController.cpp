//
// Created by Sidorenko Nikita on 2018-12-22.
//

#include "PlayerController.h"
#include "render/texture/Texture.h"
#include "loader/TextureLoader.h"
#include "render/material/Material.h"
#include "objects/LightObject.h"
#include "Engine.h"
#include "system/Input.h"

std::shared_ptr<core::Device::Texture> PlayerController::diffuse;
std::shared_ptr<core::Device::Texture> PlayerController::normal;
std::shared_ptr<core::Device::Texture> PlayerController::specular;

const float MAX_SPEED = 9.0f;
const vec3 DIRECTION_LEFT = vec3(1, 0, -1);
const vec3 DIRECTION_RIGHT = -DIRECTION_LEFT;
const vec3 DIRECTION_TOP = vec3(1, 0, 1);
const vec3 DIRECTION_BOTTOM = -DIRECTION_TOP;

using namespace core;
using namespace core::system;

void PlayerController::start() {
  if (!diffuse) {
    diffuse = loader::LoadTexture("resources/models/dwarf/dwarf_texture_diffuse.jpg");
    normal = loader::LoadTexture("resources/models/dwarf/dwarf_texture_normal.jpg", false);
    specular = loader::LoadTexture("resources/models/dwarf/dwarf_texture_specular.jpg");
  }

  _runPlayback = animation()->getPlayback("run");
  _idlePlayback = animation()->getPlayback("idle");

  _runPlayback->play(true);
  _idlePlayback->play(true);
  _runPlayback->weight(0);

  auto material = std::make_shared<Material>();
  material->texture0(diffuse);
  //material->normalMap(normal);
  //material->specularMap(specular);
  _material = material;

  transform()->position(vec3(20, 0, 20));

  _topLight = CreateGameObject<LightObject>();
  _topLight->transform()->parent(transform());
  _topLight->transform()->position(vec3(0, 10 / transform()->scale().x, 0));
  _topLight->transform()->rotate(vec3(1, 0, 0), RAD(90));
  _topLight->transform()->scale(vec3(1) / transform()->scale()); // light scale has to be uniform
  _topLight->type(LightObjectType::Point);
  _topLight->coneAngle(110);
  _topLight->radius(21);
  _topLight->castShadows(true);
  _topLight->attenuation(3.5);

  _cullingData.type = CullingData::Type::Sphere;
  _cullingData.sphere.radius = 4;
}

void PlayerController::update(float dt) {
	auto input = Engine::Get()->GetInput();

  bool shouldSlowDown = true;

  vec3 acceleration = vec3(0);

  if (_controlsEnabled) {
    if (input->keyDown(Key::S)) {
      acceleration += DIRECTION_LEFT;
      shouldSlowDown = false;
    }

    if (input->keyDown(Key::W)) {
      acceleration += DIRECTION_RIGHT;
      shouldSlowDown = false;
    }

    if (input->keyDown(Key::D)) {
      acceleration += DIRECTION_TOP;
      shouldSlowDown = false;
    }

    if (input->keyDown(Key::A)) {
      acceleration += DIRECTION_BOTTOM;
      shouldSlowDown = false;
    }

	acceleration = -acceleration;
  }

  if (dt > 0) {
    float val = 10.0f * dt;
    _speed *= fmaxf(1 - val, 0);
  }

  auto distance = glm::distance2(acceleration, vec3(0));

  if (!shouldSlowDown && distance > 0.001) {
    _acceleration = glm::normalize(acceleration) * 100.0f;
  } else {
    _acceleration = vec3(0);
  }

  transform()->setPosition(transform()->position() + _speed * dt);
  _speed += _acceleration * dt;

  float sqSpeed = _speed.x * _speed.x + _speed.z * _speed.z;
  if (sqSpeed > 0.001) {
    auto angle = (float)(atan2(_speed.z, -_speed.x) + M_PI / 2);
    transform()->rotation(glm::angleAxis(angle, vec3(0, 1, 0)));
    auto maxSpeed = MAX_SPEED;
    auto speed = sqrtf(sqSpeed);
    if (speed > MAX_SPEED) {
      _speed = _speed / speed * MAX_SPEED;
    }

    auto runWeight = speed / maxSpeed;
    _runPlayback->weight(runWeight);
    _idlePlayback->weight(1 - runWeight);
  }

  _cullingData.sphere.position = transform()->worldPosition(true);

}
