#pragma once

#include "render/renderer/ICameraParamsProvider.h"

namespace render
{
	class ShadowMap;
}

class IShadowCaster : public ICameraParamsProvider {
public:
  friend class render::ShadowMap;

  ~IShadowCaster() override = default;
  virtual bool castShadows() const = 0;

protected:
  virtual void viewport(vec4 value) = 0;
  virtual vec4 viewport() const = 0;
};

