#pragma once

#include "CommonIncludes.h"
#include "RenderOperation.h"

enum class RenderMode : int {
  DepthOnly = 0,
  Normal, // Depth test, no depth write,
  NormalDepthWrite, // To use without depth pre pass
  UI
};

enum class RenderQueue : int {
  Opaque = 0,
  Translucent,
  Debug,
  UI,
  Count
};

class IRenderer {
public:
  virtual ~IRenderer() = default;
  virtual void AddRenderOperation(core::Device::RenderOperation &rop, RenderQueue queue) = 0;
  //virtual void RenderMesh(MeshPtr mesh) = 0;
};
