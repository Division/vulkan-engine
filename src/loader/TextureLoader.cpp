//
// Created by Sidorenko Nikita on 3/25/18.
//

#include "TextureLoader.h"
#include "system/Logging.h"
#include "EngineImage.h"

TexturePtr loader::loadTexture(const std::string &name, bool sRGB) {
  int32_t w, h, channels;
  auto data = stbi_load(name.c_str(), &w, &h, &channels, 4);

  ENGLog("Loading texture %s", name.c_str());
  TexturePtr tex = std::make_shared<Texture>();

  tex->initTexture2D(w, h, channels, sRGB, data);

  return tex;
};