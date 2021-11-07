#include "VulkanRenderState.h"
#include "VulkanDescriptorCache.h"
#include "Engine.h"
#include "render/device/VkObjects.h"
#include "utils/Math.h"
#include "render/mesh/Mesh.h"
#include "render/buffer/VulkanBuffer.h"
#include "render/buffer/ConstantBuffer.h"
#include "render/device/VulkanRenderPass.h"
#include "render/device/VulkanPipeline.h"
#include "render/device/VulkanRenderTarget.h"
#include "render/renderer/SceneRenderer.h"
#include "render/shader/ShaderBindings.h"
#include "render/shader/ShaderResource.h"
#include "render/shader/ShaderDefines.h"
#include "render/mesh/Mesh.h"
#include "render/texture/Texture.h"
#include "ecs/components/DrawCall.h"

namespace Device {

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

	void RenderMode::SetPolygonMode(PolygonMode mode)
	{
		polygon_mode = mode;
		hash_dirty = true;
	}

	void RenderMode::SetPrimitiveTopology(PrimitiveTopology topology)
	{
		primitive_topology = topology;
		hash_dirty = true;
	}

	uint32_t RenderMode::GetHash() const
	{
		if (hash_dirty)
		{
			std::array<uint32_t, 14> elements;
			elements[0]  = (int)cull_mode;
			elements[1]  = (int)depth_test_enabled;
			elements[2]  = (int)depth_write_enabled;
			elements[3]  = (int)alpha_blend_enabled;
			elements[4]  = (int)depth_func;
			elements[5]  = (int)alpha_func;
			elements[6]  = (int)blend;
			elements[7]  = (int)blend_alpha;
			elements[8]  = (int)src_blend;
			elements[9]  = (int)dst_blend;
			elements[10] = (int)src_blend_alpha;
			elements[11] = (int)dst_blend_alpha;
			elements[12] = (int)polygon_mode;
			elements[13] = (int)primitive_topology;
			hash = FastHash(elements.data(), sizeof(elements));
			hash_dirty = false;
		}
			
		return hash;
	}

	VulkanRenderState::VulkanRenderState() 
		: current_render_mode()
	{
		auto context = Engine::Get()->GetContext();
		command_pools[context->GetQueueFamilyIndex(PipelineBindPoint::Graphics)] = std::make_unique<VulkanCommandPool>(context->GetQueueFamilyIndex(PipelineBindPoint::Graphics), "Graphics RenderState");
		command_pools[context->GetQueueFamilyIndex(PipelineBindPoint::Compute)] = std::make_unique<VulkanCommandPool>(context->GetQueueFamilyIndex(PipelineBindPoint::Compute), "Compute RenderState");
		auto& device = Engine::GetVulkanDevice();

		for (auto& it : command_pools)
			for (int i = 0; i < caps::MAX_FRAMES_IN_FLIGHT; i++)
			{
				command_buffers[it.first][i] = it.second->GetCommandBuffer();
			}

		const unsigned max_count = 10000;
		const unsigned SamplerSlotCount = 20;
		
		std::array<vk::DescriptorPoolSize, 8> pool_sizes = {
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, (int)ShaderBufferName::Count * max_count),
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformTexelBuffer, (int)ShaderTextureName::Count * max_count),
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, SamplerSlotCount * max_count),
			vk::DescriptorPoolSize(vk::DescriptorType::eSampler, SamplerSlotCount * max_count),
			vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, (int)ShaderSamplerName::Count * max_count),
			vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, (int)ShaderTextureName::Count * max_count),
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, (int)ShaderTextureName::Count * max_count),
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageBufferDynamic, (int)ShaderBufferName::Count * max_count)
		};

		const auto descriptor_pool_info = vk::DescriptorPoolCreateInfo(
			vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, max_count, (uint32_t)pool_sizes.size(), pool_sizes.data()
		);

		descriptor_pool = Engine::GetVulkanDevice().createDescriptorPoolUnique(descriptor_pool_info);
	}

	VulkanRenderState::~VulkanRenderState()
	{

	}

	VulkanCommandBuffer* VulkanRenderState::GetCurrentCommandBuffer() const 
	{ 
		auto* context = Engine::Get()->GetContext();
		return command_buffers.at(context->GetQueueFamilyIndex(pipeline_bind_point))[current_frame]; 
	}

	void VulkanRenderState::UpdateGlobalDescriptorSet()
	{
		const auto new_layout_hash = current_shader->GetDescriptorSetLayout(DescriptorSetType::Global)->layout_hash;
		if (!global_bindings || !global_constants || new_layout_hash == global_layout_hash) return;

		const DescriptorSetBindings global_shader_bindings(*global_bindings, *current_shader->GetDescriptorSetLayout(DescriptorSetType::Global));
		SetDescriptorSetBindings(global_shader_bindings, *global_constants);
		
		global_layout_hash = new_layout_hash;
	}

	void VulkanRenderState::UpdateState()
	{
		if (dirty_flags == 0)
			return;

		bool update_pipeline = false;

		if (dirty_flags & (int)DirtyFlags::VertexLayout)
		{
			update_pipeline = true;
		}

		if (dirty_flags & (int)DirtyFlags::RenderMode)
		{
			update_pipeline = true;
		}

		if (dirty_flags & (int)DirtyFlags::RenderPass)
		{
			auto attachment_count = std::min(current_render_target->GetColorAttachmentCount() + current_render_target->HasDepth() ? 1u : 0u, (uint32_t)clear_values.size());

			vk::RenderPassBeginInfo render_pass_begin_info(
				current_render_pass->GetRenderPass(),
				current_render_target->GetFramebuffer(),
				vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(current_render_target->GetWidth(), current_render_target->GetHeight())),
				attachment_count, clear_values.data()
			);

			auto command_buffer = GetCurrentCommandBuffer()->GetCommandBuffer();
			command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
			render_pass_started = true;

			update_pipeline = true;
		}

		if (dirty_flags & (int)DirtyFlags::Shader)
		{
			update_pipeline = true;
		}

		if (update_pipeline)
		{
			if (!current_render_pass || !current_shader || !current_vertex_layout)
				throw new std::runtime_error("RenderState not ready");

			VulkanPipelineInitializer initializer(current_shader, current_render_pass, current_vertex_layout, &current_render_mode);
			current_pipeline = GetPipeline(initializer);

			auto command_buffer = GetCurrentCommandBuffer()->GetCommandBuffer();
			command_buffer.bindPipeline( (vk::PipelineBindPoint)pipeline_bind_point, current_pipeline->GetPipeline());
		}

		if (dirty_flags & (int)DirtyFlags::GlobalDescriptorSet)
		{
			UpdateGlobalDescriptorSet();
		}

		if (dirty_flags & (int)DirtyFlags::Viewport)
		{
			assert(current_viewport.w > 0 && current_viewport.z > 0);
			auto command_buffer = GetCurrentCommandBuffer()->GetCommandBuffer();
			vk::Viewport viewport(current_viewport.x, current_viewport.y, current_viewport.z, current_viewport.w, 0.0f, 1.0f);
			command_buffer.setViewport(0, 1, &viewport);
		}

		if (dirty_flags & (int)DirtyFlags::Scissor)
		{
			assert(current_scissor.w > 0 && current_scissor.z > 0);
			auto command_buffer = GetCurrentCommandBuffer()->GetCommandBuffer();
			vk::Rect2D scissor(vk::Offset2D(current_scissor.x, current_scissor.y), vk::Extent2D(current_scissor.z, current_scissor.w));
			command_buffer.setScissor(0, 1, &scissor);
		}

		dirty_flags = 0;
	}

	void VulkanRenderState::SetClearValue(uint32_t index, vk::ClearValue value)
	{
		clear_values[index] = value;
	}
	
	void VulkanRenderState::SetVertexLayout(const VertexLayout& layout)
	{
		if (!current_vertex_layout || current_vertex_layout->GetHash() != layout.GetHash())
		{
			dirty_flags |= (int)DirtyFlags::VertexLayout;
			current_vertex_layout = &layout;
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
		if (current_viewport != viewport)
		{
			dirty_flags |= (int)DirtyFlags::Viewport;
			current_viewport = viewport;
		}
	}

	void VulkanRenderState::SetScissor(vec4 scissor)
	{
		if (current_scissor != scissor)
		{
			dirty_flags |= (int)DirtyFlags::Scissor;
			current_scissor = scissor;
		}
	}

	void VulkanRenderState::SetShader(const ShaderProgram& program)
	{
		if (current_shader != &program)
		{
			dirty_flags |= (int)DirtyFlags::Shader;
			current_shader = &program;

			auto* global_descriptor_set_data = program.GetDescriptorSetLayout(DescriptorSetType::Global);
			if (global_bindings && !global_descriptor_set_data->Empty() && global_layout_hash != global_descriptor_set_data->layout_hash)
			{
				dirty_flags |= (int)DirtyFlags::GlobalDescriptorSet;
			}
		}
	}

	vk::Sampler VulkanRenderState::GetSampler(const SamplerMode& sampler_mode)
	{
		auto hash = sampler_mode.GetHash();
		auto iter = sampler_cache.find(hash);
		if (iter != sampler_cache.end())
			return iter->second.get();

		// todo: proper sampler creation
		vk::SamplerCreateInfo sampler_info(
			{}, vk::Filter::eLinear, vk::Filter::eLinear, vk::SamplerMipmapMode::eNearest,
			vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat, vk::SamplerAddressMode::eRepeat,
			0.0f, true, 16, false, vk::CompareOp::eNever, 0.0f, VK_LOD_CLAMP_NONE);

		auto device = Engine::GetVulkanDevice();
		auto sampler = device.createSamplerUnique(sampler_info);
		auto result = sampler.get();
		sampler_cache[hash] = std::move(sampler);
		return result;
	}

	void VulkanRenderState::SetGlobalBindings(const ResourceBindings& bindings, const ConstantBindings& constants)
	{
		global_bindings = bindings;
		global_constants = constants;
		global_layout_hash = 0;
		dirty_flags |= (int)DirtyFlags::GlobalDescriptorSet;
	}

	void VulkanRenderState::RemoveGlobalBindings()
	{
		global_layout_hash = 0;
		global_bindings = std::nullopt;
		global_constants = std::nullopt;
	}

	void VulkanRenderState::SetDescriptorSetBindings(const DescriptorSetBindings& bindings, const ConstantBindings& constant_bindings)
	{
		assert(current_pipeline);
		auto descriptor_cache = Engine::GetVulkanContext()->GetDescriptorCache();
		auto descriptor_set = descriptor_cache->GetDescriptorSet(bindings);
		auto command_buffer = GetCurrentCommandBuffer()->GetCommandBuffer();
		const auto descriptor_set_index = bindings.GetDescriptorSetLayout().set_index;

		auto& dynamic_buffer_bindings = bindings.GetDynamicBufferBindings();
		utils::SmallVector<uint32_t, 10> dynamic_offsets;

		for (auto& dynamic_binding : dynamic_buffer_bindings)
		{
			const auto offset = dynamic_binding.FlushConstantBuffer(constant_bindings);
			dynamic_offsets.push_back(offset);
		}

		command_buffer.bindDescriptorSets((vk::PipelineBindPoint)pipeline_bind_point, current_pipeline->GetPipelineLayout(), descriptor_set_index, 1u, &descriptor_set->GetVKDescriptorSet(), dynamic_offsets.size(), dynamic_offsets.data());
	}

	void VulkanRenderState::SetDescriptorSet(const DescriptorSet& descriptor_set, uint32_t index, uint32_t dynamic_offset_count, const uint32_t* dynamic_offsets)
	{
		assert(current_pipeline);

		auto command_buffer = GetCurrentCommandBuffer()->GetCommandBuffer();
		command_buffer.bindDescriptorSets((vk::PipelineBindPoint)pipeline_bind_point, current_pipeline->GetPipelineLayout(), index, 1u, &descriptor_set.GetVKDescriptorSet(), dynamic_offset_count, dynamic_offsets);
	}

	void VulkanRenderState::PushConstants(ShaderProgram::Stage stage, uint32_t offset, uint32_t size, void* data)
	{
		UpdateState();
		
		auto command_buffer = GetCurrentCommandBuffer()->GetCommandBuffer();
		vk::ShaderStageFlags vk_stage;

		switch (stage)
		{
		case ShaderProgram::Stage::Compute:
			vk_stage = vk::ShaderStageFlagBits::eCompute;
			break;

		case ShaderProgram::Stage::Fragment:
			vk_stage = vk::ShaderStageFlagBits::eFragment;
			break;

		case ShaderProgram::Stage::Vertex:
			vk_stage = vk::ShaderStageFlagBits::eVertex;
			break;
		}

		command_buffer.pushConstants(current_pipeline->GetPipelineLayout(), vk_stage, offset, size, data);
	}

	void VulkanRenderState::RenderDrawCall(const ECS::components::DrawCall* draw_call, bool is_depth)
	{
		auto* shader = is_depth ? draw_call->depth_only_shader : draw_call->shader;
		if (!shader->Ready())
			return;

		auto* mesh = draw_call->mesh;
		SetVertexLayout(mesh->GetVertexLayout());

		SetShader(*shader);
		UpdateState();

		auto command_buffer = GetCurrentCommandBuffer()->GetCommandBuffer();
		if (draw_call->descriptor_set)
		{
			auto descriptor_set = is_depth ? draw_call->depth_only_descriptor_set : draw_call->descriptor_set;
			auto& bindings = descriptor_set->GetBindings();
			auto& dynamic_buffer_bindings = bindings.GetDynamicBufferBindings();
			utils::SmallVector<uint32_t, 10> dynamic_offsets;

			for (auto& dynamic_binding : dynamic_buffer_bindings)
			{
				const auto offset = dynamic_binding.FlushConstantBuffer(draw_call->constants);
				dynamic_offsets.push_back(offset);
			}

			SetDescriptorSet(*descriptor_set, DescriptorSetType::Object, dynamic_offsets.size(), dynamic_offsets.data());
		}

		if (mesh->hasIndices())
		{
			const auto index_type = Mesh::IsShortIndexCount(mesh->indexCount()) ? IndexType::UINT16 : IndexType::UINT32;
			if (draw_call->indirect_buffer)
				DrawIndexedIndirect(*mesh->vertexBuffer(), *mesh->indexBuffer(), index_type, *draw_call->indirect_buffer, draw_call->indirect_buffer_offset);
			else
				DrawIndexed(*mesh->vertexBuffer(), *mesh->indexBuffer(), 0, mesh->indexCount(), 0, index_type, draw_call->instance_count);
		}
		else
		{
			if (draw_call->indirect_buffer)
				DrawIndirect(*mesh->vertexBuffer(), *draw_call->indirect_buffer, draw_call->indirect_buffer_offset);
			else
				Draw(*mesh->vertexBuffer(), mesh->indexCount(), 0, draw_call->instance_count);
		}
	}

	void VulkanRenderState::Draw(const VulkanBuffer& buffer, uint32_t vertex_count, uint32_t first_vertex, uint32_t instance_count)
	{
		UpdateState();

		auto command_buffer = GetCurrentCommandBuffer()->GetCommandBuffer();
		vk::DeviceSize offset = { 0 };
		vk::Buffer vertex_buffer = buffer.Buffer();
		command_buffer.bindVertexBuffers(0, 1, &vertex_buffer, &offset);
		command_buffer.draw(vertex_count, instance_count, first_vertex, 0);
	}

	void VulkanRenderState::DrawIndirect(const VulkanBuffer& buffer, VulkanBuffer& indirect_buffer, uint32_t indirect_buffer_offset)
	{
		UpdateState();

		auto command_buffer = GetCurrentCommandBuffer()->GetCommandBuffer();
		vk::DeviceSize offset = { 0 };
		vk::Buffer vertex_buffer = buffer.Buffer();
		command_buffer.bindVertexBuffers(0, 1, &vertex_buffer, &offset);
		command_buffer.drawIndirect(indirect_buffer.Buffer(), indirect_buffer_offset, 1, 0);
	}

	void VulkanRenderState::DrawIndexed(const VulkanBuffer& vertex_buffer, const VulkanBuffer& index_buffer, uint32_t vertex_offset, uint32_t index_count, uint32_t first_index, IndexType index_type, uint32_t instance_count)
	{
		UpdateState();

		auto command_buffer = GetCurrentCommandBuffer()->GetCommandBuffer();
		vk::DeviceSize offset = { 0 };
		vk::Buffer vk_vertex_buffer = vertex_buffer.Buffer();
		vk::Buffer vk_index_buffer = index_buffer.Buffer();
		command_buffer.bindIndexBuffer(vk_index_buffer, offset, index_type == IndexType::UINT16 ? vk::IndexType::eUint16 : vk::IndexType::eUint32);
		command_buffer.bindVertexBuffers(0, 1, &vk_vertex_buffer, &offset);
		command_buffer.drawIndexed(index_count, instance_count, first_index, vertex_offset, 0);
	}

	void VulkanRenderState::DrawIndexedIndirect(const VulkanBuffer& vertex_buffer, const VulkanBuffer& index_buffer, IndexType index_type, VulkanBuffer& indirect_buffer, uint32_t indirect_buffer_offset)
	{
		UpdateState();

		auto command_buffer = GetCurrentCommandBuffer()->GetCommandBuffer();
		vk::DeviceSize offset = { 0 };
		vk::Buffer vk_vertex_buffer = vertex_buffer.Buffer();
		vk::Buffer vk_index_buffer = index_buffer.Buffer();
		command_buffer.bindIndexBuffer(vk_index_buffer, offset, index_type == IndexType::UINT16 ? vk::IndexType::eUint16 : vk::IndexType::eUint32);
		command_buffer.bindVertexBuffers(0, 1, &vk_vertex_buffer, &offset);
		command_buffer.drawIndexedIndirect(indirect_buffer.Buffer(), indirect_buffer_offset, 1, 0);
	}

	void VulkanRenderState::BeginRecording(PipelineBindPoint bind_point)
	{
		pipeline_bind_point = bind_point;
		auto begin_info = vk::CommandBufferBeginInfo();
		auto command_buffer = GetCurrentCommandBuffer()->GetCommandBuffer();
		command_buffer.begin(begin_info);
	}

	VulkanCommandBuffer* VulkanRenderState::BeginRendering(const VulkanRenderTarget& render_target, const VulkanRenderPass& render_pass)
	{
		dirty_flags = (uint32_t)DirtyFlags::All;
		pipeline_bind_point = PipelineBindPoint::Graphics;
		current_render_pass = nullptr;
		current_render_mode = RenderMode();
		current_vertex_layout = nullptr;
		current_shader = nullptr;
		current_pipeline = nullptr;
		current_render_target = &render_target;
		render_pass_started = false;
		global_layout_hash = 0;
		global_bindings = std::nullopt;

		SetRenderPass(render_pass);

		return GetCurrentCommandBuffer();
	}

	void VulkanRenderState::EndRendering()
	{
		if (render_pass_started)
		{
			auto command_buffer = GetCurrentCommandBuffer()->GetCommandBuffer();
			command_buffer.endRenderPass();
		}
	}

	void VulkanRenderState::EndRecording()
	{
		auto command_buffer = GetCurrentCommandBuffer()->GetCommandBuffer();
		command_buffer.end();
		current_frame = (current_frame + 1) % caps::MAX_FRAMES_IN_FLIGHT;
	}

	void VulkanRenderState::Dispatch(const ShaderProgram& program, ResourceBindings& bindings, ConstantBindings& constants, uvec3 group_size)
	{
		render_pass_started = true;

		VulkanPipelineInitializer compute_pipeline_initializer(&program);
		current_pipeline = GetPipeline(compute_pipeline_initializer);
		auto command_buffer = GetCurrentCommandBuffer()->GetCommandBuffer();
		command_buffer.bindPipeline( vk::PipelineBindPoint::eCompute, current_pipeline->GetPipeline());

		const DescriptorSetBindings global_shader_bindings(bindings, *program.GetDescriptorSetLayout(DescriptorSetType::Global));
		SetDescriptorSetBindings(global_shader_bindings, constants);

		vkCmdDispatch(command_buffer, group_size.x, group_size.y, group_size.z);

		dirty_flags = (uint32_t)DirtyFlags::All;
	}

	void VulkanRenderState::DispatchIndirect(const ShaderProgram& program, ResourceBindings& bindings, ConstantBindings& constants, VulkanBuffer& buffer, uint32_t offset)
	{
		render_pass_started = true;

		VulkanPipelineInitializer compute_pipeline_initializer(&program);
		current_pipeline = GetPipeline(compute_pipeline_initializer);
		auto command_buffer = GetCurrentCommandBuffer()->GetCommandBuffer();
		command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, current_pipeline->GetPipeline());

		const DescriptorSetBindings global_shader_bindings(bindings, *program.GetDescriptorSetLayout(DescriptorSetType::Global));
		SetDescriptorSetBindings(global_shader_bindings, constants);

		vkCmdDispatchIndirect(command_buffer, buffer.Buffer(), offset);

		dirty_flags = (uint32_t)DirtyFlags::All;
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

}