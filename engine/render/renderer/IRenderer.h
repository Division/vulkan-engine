#pragma once

#include "CommonIncludes.h"

enum class RenderQueue : int {
  DepthOnly,
  Opaque,
  AlphaTest,
  Translucent,
  Additive,
  Debug,
  UI,
  Count
};

class IRenderer {
public:
  virtual ~IRenderer() = default;
};
