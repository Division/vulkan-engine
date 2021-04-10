#pragma once

#include "CommonIncludes.h"
#include "DDS.h"
#include "render/texture/Texture.h"

namespace loader 
{

	std::unique_ptr<Device::Texture> LoadDDSFromMemory(const std::vector<uint8_t>& data, bool sRGB = true);

}