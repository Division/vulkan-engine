#pragma once

#include "CommonIncludes.h"

enum class RendererMode : int {
  DepthOnly = 0,
  Normal, // Depth test, no depth write,
  NormalDepthWrite, // To use without depth pre pass
  UI
};

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
  //virtual void RenderMesh(MeshPtr mesh) = 0;
};
