#include "VulkanRenderState.h"
#include "Engine.h"
#include "render/device/VkObjects.h"
#include "utils/Math.h"
#include "render/shader/Shader.h"
#include "render/mesh/Mesh.h"
#include "render/buffer/VulkanBuffer.h"
#include "render/device/VulkanRenderPass.h"
#include "render/device/VulkanPipeline.h"
#include "render/device/VulkanRenderTarget.h"
#include "render/renderer/SceneRenderer.h"

namespace core { namespace Device {

	void RenderMode::SetCullMode(CullMode cull_mode)
	{
		this->cull_mode = cull_mode;
	}

	void RenderMode::SetDepthTestEnabled(bool enabled)
	{
		depth_test_enabled = enabled;
		hash_dirty = true;
	}

	void RenderMode::SetDepthWriteEnabled(bool enabled)
	{
		depth_write_enabled = enabled;
		hash_dirty = true;
	}

	void RenderMode::SetAlphaBlendEnabled(bool enabled)
	{
		alpha_blend_enabled = enabled;
		hash_dirty = true;
	}

	void RenderMode::SetDepthFunc(CompareOp compare_mode)
	{
		depth_func = compare_mode;
		hash_dirty = true;
	}

	void RenderMode::SetAlphaFunc(CompareOp compare_mode)
	{
		alpha_func = compare_mode;
		hash_dirty = true;
	}

	void RenderMode::SetBlend(BlendOp blend_op)
	{
		blend = blend_op;
		hash_dirty = true;
	}

	void RenderMode::SetBlendAlpha(BlendOp blend_op)
	{
		blend_alpha = blend_op;
		hash_dirty = true;
	}

	void RenderMode::SetSrcBlend(BlendFactor blend_mode)
	{
		src_blend = blend_mode;
		hash_dirty = true;
	}

	void RenderMode::SetDestBlend(BlendFactor blend_mode)
	{
		dst_blend = blend_mode;
		hash_dirty = true;
	}

	void RenderMode::SetSrcBlendAlpha(BlendFactor blend_mode)
	{
		src_blend_alpha = blend_mode;
		hash_dirty = true;
	}

	void RenderMode::SetDestBlendAlpha(BlendFactor blend_mode)
	{
		dst_blend_alpha = blend_mode;
		hash_dirty = true;
	}

	uint32_t RenderMode::GetHash() const
	{
		if (hash_dirty)
		{
			std::array<uint32_t, 12> elements;
			elements[0]  = (int)cull_mode;
			elements[1]  = (int)depth_test_enabled;
			elements[2]  = (int)depth_write_enabled;
			elements[3]  = (int)alpha_blend_enabled;
			elements[4]  = (int)depth_func;
			elements[5]  = (int)alpha_func;
			elements[6]  = (int)blend;
			elements[7]  = (int)blend_alpha;
			elements[8]  = (int)src_blend;
			elements[9] = (int)dst_blend;
			elements[10] = (int)src_blend_alpha;
			elements[11] = (int)dst_blend_alpha;
			hash = FastHash(elements.data(), sizeof(elements));
			hash_dirty = false;
		}
			
		return hash;
	}


	VulkanRenderState::VulkanRenderState() 
		: current_render_mode()
	{
		command_pool = std::make_unique<core::Device::VulkanCommandPool>();

		for (int i = 0; i < caps::MAX_FRAMES_IN_FLIGHT; i++)
		{
			command_buffers[i] = command_pool->GetCommandBuffer();
		}
	}

	VulkanRenderState::~VulkanRenderState()
	{

	}

	void VulkanRenderState::UpdateState()
	{
		if (dirty_flags == 0)
			return;

		bool update_pipeline = false;

		if (dirty_flags & (int)DirtyFlags::Mesh)
		{
			update_pipeline = true;
		}

		if (dirty_flags & (int)DirtyFlags::RenderMode)
		{
			update_pipeline = true;
		}

		if (dirty_flags & (int)DirtyFlags::RenderPass)
		{
			auto* context = Engine::GetVulkanContext();
			vk::ClearValue clear_color(vk::ClearColorValue(std::array<float, 4>{ 1.0f, 1.0f, 1.0f, 1.0f }));
			vk::RenderPassBeginInfo render_pass_begin_info(
				current_render_pass->GetRenderPass(),
				current_render_target->GetFrame(context->GetCurrentFrame()).framebuffer.get(),
				vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(current_render_target->GetWidth(), current_render_target->GetHeight())),
				1, &clear_color
			);

			auto command_buffer = GetCurrentCommandBuffer()->GetCommandBuffer();
			command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

			update_pipeline = true;
		}

		if (dirty_flags & (int)DirtyFlags::Shader)
		{
			update_pipeline = true;
		}

		if (update_pipeline)
		{
			if (!current_render_pass || !current_shader || !current_mesh)
				throw new std::runtime_error("RenderState not ready");

			VulkanPipelineInitializer initializer(current_shader, current_render_pass, current_mesh, &current_render_mode);
			current_pipeline = GetPipeline(initializer);

			auto command_buffer = GetCurrentCommandBuffer()->GetCommandBuffer();
			command_buffer.bindPipeline( vk::PipelineBindPoint::eGraphics, current_pipeline->GetPipeline());
		}

		dirty_flags = 0;
	}
	
	void VulkanRenderState::SetMesh(const Mesh& mesh)
	{
		if (!current_mesh || current_mesh->GetVertexAttribHash() != mesh.GetVertexAttribHash())
		{
			dirty_flags |= (int)DirtyFlags::Mesh;
			current_mesh = &mesh;
		}
	}

	void VulkanRenderState::SetRenderMode(const RenderMode& mode)
	{
		if (current_render_mode.GetHash() != mode.GetHash())
		{
			current_render_mode = mode;
			dirty_flags |= (int)DirtyFlags::RenderMode;
		}
	}

	void VulkanRenderState::SetRenderPass(const VulkanRenderPass& render_pass)
	{
		if (current_render_pass != &render_pass)
		{
			dirty_flags |= (int)DirtyFlags::RenderPass;
			current_render_pass = &render_pass;
		}
	}

	void VulkanRenderState::SetViewport(vec4 viewport)
	{

	}

	void VulkanRenderState::SetShader(const ShaderProgram& program)
	{
		if (current_shader != &program)
		{
			dirty_flags |= (int)DirtyFlags::Shader;
			current_shader = &program;
		}
	}

	void VulkanRenderState::RenderMesh(const Mesh& mesh)
	{
		SetMesh(mesh);
		UpdateState();

		if (!mesh.hasIndices())
		{
			throw std::runtime_error("mesh should have indices");
		}
		
		vk::DeviceSize offset = { 0 };
		vk::Buffer vertex_buffer = mesh.vertexBuffer()->Buffer();
		vk::Buffer index_buffer = mesh.indexBuffer()->Buffer();
		auto command_buffer = GetCurrentCommandBuffer()->GetCommandBuffer();
		command_buffer.bindVertexBuffers(0, 1, &vertex_buffer, &offset);
		command_buffer.bindIndexBuffer(index_buffer, offset, vk::IndexType::eUint16);

		command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, current_pipeline->GetPipelineLayout(), 0u, 1u, &core::render::descriptorSets[current_frame], 0, nullptr);
		command_buffer.drawIndexed(static_cast<uint32_t>(mesh.indexCount()), 1, 0, 0, 0);
	}

	VulkanCommandBuffer* VulkanRenderState::BeginRendering(const VulkanRenderTarget& render_target)
	{
		dirty_flags = (uint32_t)DirtyFlags::All;
		current_frame = (current_frame + 1) % caps::MAX_FRAMES_IN_FLIGHT;
		current_render_pass = nullptr;
		current_render_mode = RenderMode();
		current_mesh = nullptr;
		current_shader = nullptr;
		current_pipeline = nullptr;
		current_render_target = &render_target;

		SetRenderPass(*render_target.GetRenderPass());

		auto begin_info = vk::CommandBufferBeginInfo();
		auto command_buffer = GetCurrentCommandBuffer()->GetCommandBuffer();
		command_buffer.begin(begin_info);

		return GetCurrentCommandBuffer();
	}

	void VulkanRenderState::EndRendering()
	{
		auto command_buffer = GetCurrentCommandBuffer()->GetCommandBuffer();
		command_buffer.endRenderPass();
		command_buffer.end();
	}

	VulkanPipeline* VulkanRenderState::GetPipeline(const VulkanPipelineInitializer& initializer)
	{
		auto iter = pipeline_cache.find(initializer.GetHash());

		if (iter != pipeline_cache.end())
			return iter->second.get();

		auto pipeline = std::make_unique<VulkanPipeline>(initializer);
		auto result = pipeline.get();
		pipeline_cache[initializer.GetHash()] = std::move(pipeline);
		return result;
	}

} }