//
// Created by Sidorenko Nikita on 7/14/18.
//

#ifndef CPPWRAPPER_SPRITESHEETLOADER_H
#define CPPWRAPPER_SPRITESHEETLOADER_H

#include <string>
#include <memory>

class SpriteSheet;

namespace loader {

  std::shared_ptr<SpriteSheet> loadSpritesheet(const std::string &filename);

}



#endif //CPPWRAPPER_SPRITESHEETLOADER_H
