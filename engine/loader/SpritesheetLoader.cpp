//
// Created by Sidorenko Nikita on 7/14/18.
//

#include "SpritesheetLoader.h"
#include <fstream>
#include <sstream>
#include "render/texture/SpriteSheet.h"
#include "system/Logging.h"

std::shared_ptr<SpriteSheet> loader::loadSpritesheet(const std::string &filename) {
  std::ifstream file(filename);
  std::string jsonString((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

  json jsonData = json::parse(jsonString);

  auto spritesheet = std::make_shared<SpriteSheet>();
  spritesheet->loadFromJSON(jsonData);
  return spritesheet;
}