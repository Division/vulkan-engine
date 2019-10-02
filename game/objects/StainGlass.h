//
// Created by Sidorenko Nikita on 2018-12-30.
//

#ifndef CPPWRAPPER_STAINGLASS_H
#define CPPWRAPPER_STAINGLASS_H

#include "objects/MeshObject.h"
#include "EngineTypes.h"
#include <memory>

class StainGlass : public MeshObject {
public:

  void start() override;
  void setup(SpriteSheetPtr &spritesheet, TexturePtr &decals, const std::string &spriteName);

};


#endif //CPPWRAPPER_STAINGLASS_H
