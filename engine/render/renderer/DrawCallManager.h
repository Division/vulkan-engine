#pragma once

#include "CommonIncludes.h"

namespace core 
{ 
	namespace ECS 
	{
		class EntityManager;
	} 
}

namespace core { namespace render {

	class DrawCallManager
	{
	public:
		DrawCallManager();
		~DrawCallManager();

	private:
		std::unique_ptr<ECS::EntityManager> manager;
	};

} }