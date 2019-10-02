#pragma once

#include "CommonIncludes.h"
#include "utils/Pool.h"
#include "render/renderer/DrawCall.h"
#include "render/shader/ShaderResource.h"

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

	class SceneBuffers;

	class SceneRenderer
	{
	public:
		SceneRenderer();
		~SceneRenderer();

		void RenderScene(Scene* scene);

	private:
		DrawCall* GetDrawCall(RenderOperation& rop);
		void ReleaseDrawCalls();
		void SetupShaderBindings(RenderOperation& rop, ShaderProgram& shader, ShaderBindings& bindings);
		std::tuple<vk::Buffer, size_t, size_t> GetBufferFromROP(RenderOperation& rop, ShaderBufferName buffer_name);
	private:
		std::unique_ptr<SceneBuffers> scene_buffers;
		core::utils::Pool<DrawCall> draw_call_pool;
		std::vector<std::unique_ptr<DrawCall>> used_draw_calls;
	};

} }