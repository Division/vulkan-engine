#pragma once

#include "CommonIncludes.h"

class Scene;

namespace core { namespace render {

	class SceneRenderer
	{
	public:
		SceneRenderer();

		void RenderScene(Scene* scene);

	private:
	};

} }