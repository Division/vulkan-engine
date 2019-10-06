#pragma once 
#include "CommonIncludes.h"
#include "utils/Frustum.h"

// Scene is rendered not only from the Camera scene objects, but also from the light's point of view
// This abstraction serves as a common interface for them
class ICameraParamsProvider {
public:
  virtual ~ICameraParamsProvider() = default;
  virtual uvec2 cameraViewSize() const = 0;
  virtual vec3 cameraPosition() const = 0;
  virtual mat4 cameraViewProjectionMatrix() const = 0;
  virtual mat4 cameraViewMatrix() const = 0;
  virtual mat4 cameraProjectionMatrix() const = 0;
  virtual vec4 cameraViewport() const = 0;
  virtual const Frustum &frustum() const = 0;
  virtual unsigned int cameraIndex() const = 0; // index is an offset in the corresponding UBO
  virtual void cameraIndex(unsigned int index) = 0;
  virtual unsigned int cameraVisibilityMask() const = 0;

  // There vectors can be obtained from the view matrix but will keep it for now
  virtual vec3 cameraLeft() const = 0;
  virtual vec3 cameraRight() const = 0;
  virtual vec3 cameraUp() const = 0;
  virtual vec3 cameraDown() const = 0;
};

typedef std::shared_ptr<ICameraParamsProvider> ICameraParamsProviderPtr;
