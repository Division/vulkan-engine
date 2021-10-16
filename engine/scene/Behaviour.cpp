#include "Behaviour.h"
#include "ecs/components/BehaviourList.h"

namespace scene
{
	std::shared_ptr<Behaviour> Behaviour::GetBehaviour(size_t hash)
	{
		return behaviour_list->GetBehaviour(hash);
	}

}