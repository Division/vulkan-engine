//
// Created by Sidorenko Nikita on 4/5/18.
//

#ifndef CPPWRAPPER_MODELLOADER_H
#define CPPWRAPPER_MODELLOADER_H

#include <iostream>
#include <fstream>
#include <resources/ModelBundle.h>

namespace loader {
  ModelBundlePtr loadModel(const std::string &filename);
  ModelBundlePtr loadModel(std::istream &stream, const std::string &url);
}


#endif //CPPWRAPPER_MODELLOADER_H
