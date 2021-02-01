#include "utils/Math.h"

namespace game::components
{
	struct Sphere
	{
		enum class Type : int
		{
			Default,
			Bonus
		};

		Type type = Type::Default;
		int color = 0;
		vec3 position;
	};
}