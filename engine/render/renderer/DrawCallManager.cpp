#include "DrawCallManager.h"
#include "ecs/ECS.h"

namespace core { namespace render {

	using namespace ECS;

	DrawCallManager::~DrawCallManager() = default;

	DrawCallManager::DrawCallManager()
	{
		manager = std::make_unique<EntityManager>();
	}



} }