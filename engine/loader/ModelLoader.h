//
// Created by Sidorenko Nikita on 4/5/18.
//

#ifndef CPPWRAPPER_MODELLOADER_H
#define CPPWRAPPER_MODELLOADER_H

#include <iostream>
#include <fstream>
#include <resources/ModelBundle.h>

namespace loader 
{
	void loadModel(std::istream& stream, ModelBundle& bundle);
}


#endif //CPPWRAPPER_MODELLOADER_H
