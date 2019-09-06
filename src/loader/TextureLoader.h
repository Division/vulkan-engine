//
// Created by Sidorenko Nikita on 3/25/18.
//

#ifndef CPPWRAPPER_TEXTURELOADER_H
#define CPPWRAPPER_TEXTURELOADER_H

#include <string>
#include "render/texture/Texture.h"

namespace loader {
  TexturePtr loadTexture(const std::string &name, bool sRGB = true);
};


#endif //CPPWRAPPER_TEXTURELOADER_H
