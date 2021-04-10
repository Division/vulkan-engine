#pragma once

#include "CommonIncludes.h"
#include "render/texture/Texture.h"

namespace loader {

    Device::Format GetNoSRGBFormat(Device::Format src_format);
    std::unique_ptr<Device::Texture> LoadTexture(const std::wstring &name, bool sRGB = true);

};


