#pragma once

#include "CommonIncludes.h"

enum class RenderQueue : int {
  DepthOnly,
  Opaque,
  AlphaTest,
  Translucent,
  Debug,
  UI,
  Count
};

class IRenderer {
public:
  virtual ~IRenderer() = default;
};
