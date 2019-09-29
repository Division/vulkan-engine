#pragma once

#include "CommonIncludes.h"
#include "utils/Pool.h"
#include "render/renderer/DrawCall.h"

class Scene;

namespace core
{
	namespace Device
	{
		class ShaderBindings;
		struct RenderOperation;
	}
}

namespace core { namespace render {

	class SceneRenderer
	{
	public:
		SceneRenderer();

		void RenderScene(Scene* scene);

	private:
		DrawCall* GetDrawCall(RenderOperation& rop);
		void ReleaseDrawCalls();

	private:
		
		core::utils::Pool<DrawCall> draw_call_pool;
		std::vector<std::unique_ptr<DrawCall>> used_draw_calls;
	};

} }