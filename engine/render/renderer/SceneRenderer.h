#pragma once

#include "CommonIncludes.h"
#include "utils/Pool.h"
#include "render/renderer/DrawCall.h"
#include "render/renderer/IRenderer.h"
#include "render/shader/ShaderResource.h"

class Scene;

namespace core
{
	namespace Device
	{
		class ShaderProgram;
		class ShaderBindings;
		struct RenderOperation;
	}
}

namespace core { namespace render {

	class SceneBuffers;

	class SceneRenderer : IRenderer
	{
	public:
		SceneRenderer();
		~SceneRenderer();

		void RenderScene(Scene* scene);
		void AddRenderOperation(core::Device::RenderOperation& rop, RenderQueue queue) override;

	private:
		DrawCall* GetDrawCall(RenderOperation& rop);
		void ReleaseDrawCalls();
		void SetupShaderBindings(RenderOperation& rop, ShaderProgram& shader, ShaderBindings& bindings);
		std::tuple<vk::Buffer, size_t, size_t> GetBufferFromROP(RenderOperation& rop, ShaderBufferName buffer_name);

	private:
		std::shared_ptr<core::Device::Texture> texture;
		std::unique_ptr<SceneBuffers> scene_buffers;
		std::unique_ptr<ShaderProgram> program;
		core::utils::Pool<DrawCall> draw_call_pool;
		std::vector<std::unique_ptr<DrawCall>> used_draw_calls;
		std::unordered_map<RenderOperation*, vk::DescriptorBufferInfo> rop_transform_cache; // cleared every frame. Allows reusing same object transform buffer in multiple draw calls.
		std::array<std::vector<DrawCall*>, (size_t)RenderQueue::Count> render_queues;
	};

} }