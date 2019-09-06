//
// Created by Sidorenko Nikita on 3/30/18.
//

#ifndef CPPWRAPPER_IGAME_H
#define CPPWRAPPER_IGAME_H

#include <memory>

namespace core
{
	class IGame {
	public:
	  virtual ~IGame() = default;
	  virtual void init() = 0;
	  virtual void update(float dt) = 0;
	  virtual void cleanup() = 0;
	};

}


#endif //CPPWRAPPER_IGAME_H
