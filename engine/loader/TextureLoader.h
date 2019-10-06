#pragma once

#include "CommonIncludes.h"
#include "render/texture/Texture.h"

namespace loader {

  std::shared_ptr<core::Device::Texture> LoadTexture(const std::string &name, bool sRGB = true);

};


