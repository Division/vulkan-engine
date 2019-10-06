#include "TextureLoader.h"
#include "system/Logging.h"

using namespace core::Device;

std::shared_ptr<Texture> loader::LoadTexture(const std::string &name, bool sRGB) {
  int32_t w, h, channels;
  auto data = stbi_load(name.c_str(), &w, &h, &channels, 4);

  ENGLog("Loading texture %s", name.c_str());

  TextureInitializer initializer(w, h, 4, data, sRGB);
  auto texture = std::make_shared<Texture>(initializer);
  stbi_image_free(data);

  return texture;
};
