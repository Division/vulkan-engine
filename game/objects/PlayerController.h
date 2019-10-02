#pragma once

#include "objects/SkinnedMeshObject.h"

class LightObject;

class PlayerController : public SkinnedMeshObject {
public:
  static std::shared_ptr<core::Device::Texture> diffuse;
  static std::shared_ptr<core::Device::Texture> normal;
  static std::shared_ptr<core::Device::Texture> specular;

public:
  void start() override;
  void update(float dt) override;
  void controlsEnabled(bool enabled) { _controlsEnabled = enabled; }

private:
  bool _controlsEnabled = true;
  AnimationPlaybackPtr _runPlayback;
  AnimationPlaybackPtr _idlePlayback;
  vec3 _speed = vec3(0);
  vec3 _acceleration = vec3(0);
  std::shared_ptr<LightObject> _topLight; // spot light always above the player
};
