//
// Created by Sidorenko Nikita on 4/7/18.
//

#ifndef CPPWRAPPER_MODELLOADERUTILS_H
#define CPPWRAPPER_MODELLOADERUTILS_H

#include "CommonIncludes.h"
#include "system/Logging.h"

using namespace nlohmann;

namespace loader {

  mat4 getMatrixFromJSON(json matrix);
  void flipMatrix(mat4 &matrix);
}

#endif //CPPWRAPPER_MODELLOADERUTILS_H
