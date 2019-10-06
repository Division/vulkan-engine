#pragma once

#include "CommonIncludes.h"
#include "Texture.h"
#include "nlohmann/json.hpp"
#include "utils/Math.h"

using namespace nlohmann;

struct SpriteData {
  SpriteData() { name = ""; }
  SpriteData(const std::string &name, Rect bounds) : name(name), bounds(bounds) {}
  std::string name;
  Rect bounds;
};

class SpriteSheet {
public:
  SpriteSheet() = default;
  void addSprite(const std::string &name, float x, float y, float width, float height);
  void loadFromJSON(const json &jsonData);
  const std::string &spritesheetName() { return _spritesheetName; }
  const std::vector<std::string> &spriteNames() { return _spriteNames; };
  const SpriteData &getSpriteData(const std::string &name) { return _sprites.at(name); }

private:
  float _width = 0;
  float _height = 0;
  std::string _spritesheetName;
  std::vector<std::string> _spriteNames;
  std::unordered_map<std::string, SpriteData> _sprites;
};

typedef std::shared_ptr<SpriteSheet> SpriteSheetPtr;
