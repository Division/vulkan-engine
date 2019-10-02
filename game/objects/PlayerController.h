//
// Created by Sidorenko Nikita on 2018-12-22.
//

#ifndef CPPWRAPPER_PLAYERCONTROLLER_H
#define CPPWRAPPER_PLAYERCONTROLLER_H

#include "objects/SkinnedMeshObject.h"
#include "EngineTypes.h"


class PlayerController : public SkinnedMeshObject {
public:
  static TexturePtr diffuse;
  static TexturePtr normal;
  static TexturePtr specular;

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
  LightObjectPtr _topLight; // spot light always above the player
};


#endif //CPPWRAPPER_PLAYERCONTROLLER_H
