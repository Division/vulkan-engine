#pragma once 
#include "CommonIncludes.h"
#include "utils/Frustum.h"

// Scene is rendered not only from the Camera scene objects, but also from the light's point of view
// This abstraction serves as a common interface for them
class ICameraParamsProvider {
public:
  virtual ~ICameraParamsProvider() = default;
  virtual vec3 cameraPosition() const = 0;
  virtual mat4 cameraViewProjectionMatrix() const = 0;
  virtual mat4 cameraViewMatrix() const = 0;
  virtual mat4 cameraProjectionMatrix() const = 0;
  virtual vec2 cameraZMinMax() const = 0;
  virtual const Frustum &GetFrustum() const = 0;
  virtual unsigned int cameraVisibilityMask() const = 0;
};
