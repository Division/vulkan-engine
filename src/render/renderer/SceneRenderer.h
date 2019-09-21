#pragma once

#include "CommonIncludes.h"

class Scene;

namespace core { namespace render {

	// TODO: remove
	extern std::vector<vk::DescriptorSet> descriptorSets;

	class SceneRenderer
	{
	public:
		SceneRenderer();

		void RenderScene(Scene* scene);

	private:
	};

} }