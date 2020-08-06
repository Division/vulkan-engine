#pragma once

#include "objects/SkinnedMeshObject.h"

class LightObject;

class PlayerController : public SkinnedMeshObject {
public:
  void start() override;
  void update(float dt) override;
  void controlsEnabled(bool enabled) { _controlsEnabled = enabled; }

private:
  std::shared_ptr<Device::Texture> diffuse;
  std::shared_ptr<Device::Texture> normal;
  std::shared_ptr<Device::Texture> specular;

  bool _controlsEnabled = true;
  AnimationPlaybackPtr _runPlayback;
  AnimationPlaybackPtr _idlePlayback;
  vec3 _speed = vec3(0);
  vec3 _acceleration = vec3(0);
  std::shared_ptr<LightObject> _topLight; // spot light always above the player
};
