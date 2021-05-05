//
// Created by Sidorenko Nikita on 7/14/18.
//

#include "SpriteSheet.h"
#include <system/Logging.h>

void SpriteSheet::addSprite(const std::string &name, float x, float y, float width, float height) {
  if (_sprites.count(name)) {
    throw std::runtime_error("Sprite already exists: " + name);
  }

  auto rect = Rect(x / _width, y / _height, width / _width, height / _height);
  _sprites[name] = SpriteData(name, rect);
  _spriteNames.push_back(name);
}

void SpriteSheet::loadFromJSON(const json &jsonData) {
  /*_sprites.clear();
  _spriteNames.clear();

  auto size = jsonData["meta"]["size"];
  _width = size["w"];
  _height = size["h"];
  _spritesheetName = jsonData["meta"]["image"];

  auto frames = jsonData["frames"];
  auto frameCount = frames.size();
  for (size_t i = 0; i < frameCount; i++) {
    auto spriteData = frames.at(i);
    auto frame = spriteData["frame"];
    addSprite(spriteData["filename"], frame["x"], frame["y"], frame["w"], frame["h"]);
  }*/
}
