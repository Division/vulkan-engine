//
// Created by Sidorenko Nikita on 2018-11-29.
//

#ifndef CPPWRAPPER_ISHADOWCASTER_H
#define CPPWRAPPER_ISHADOWCASTER_H

#include "render/renderer/ICameraParamsProvider.h"
#include <memory>

class IShadowCaster : public ICameraParamsProvider {
public:
  friend class ShadowMap;

  ~IShadowCaster() override = default;
  virtual bool castShadows() const = 0;

protected:
  virtual void viewport(vec4 value) = 0;
  virtual vec4 viewport() const = 0;
};

typedef std::shared_ptr<IShadowCaster> IShadowCasterPtr;

#endif //CPPWRAPPER_ISHADOWCASTER_H
