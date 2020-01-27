#include "VulkanRenderState.h"
#include "VulkanDescriptorCache.h"
#include "Engine.h"
#include "render/device/VkObjects.h"
#include "utils/Math.h"
#include "render/mesh/Mesh.h"
#include "render/buffer/VulkanBuffer.h"
#include "render/device/VulkanRenderPass.h"
#include "render/device/VulkanPipeline.h"
#include "render/device/VulkanRenderTarget.h"
#include "render/renderer/SceneRenderer.h"
#include "render/shader/ShaderBindings.h"
#include "render/shader/ShaderResource.h"
#include "render/shader/ShaderDefines.h"
#include "render/renderer/RenderOperation.h"
#include "render/renderer/DrawCall.h"
#include "render/mesh/Mesh.h"
#include "render/texture/Texture.h"
#include "ecs/components/DrawCall.h"

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
			elements[9]  = (int)dst_blend;
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
		auto& device = Engine::GetVulkanDevice();

		for (int i = 0; i < caps::MAX_FRAMES_IN_FLIGHT; i++)
		{
			command_buffers[i] = command_pool->GetCommandBuffer();
			semaphores[i] = device.createSemaphoreUnique(vk::SemaphoreCreateInfo());
		}

		const unsigned max_count = 10000;
		const unsigned SamplerSlotCount = 20;
		
		std::array<vk::DescriptorPoolSize, 7> pool_sizes = {
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, (int)ShaderBufferName::Count * max_count),
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformTexelBuffer, (int)ShaderTextureName::Count * max_count),
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, SamplerSlotCount * max_count),
			vk::DescriptorPoolSize(vk::DescriptorType::eSampler, SamplerSlotCount * max_count),
			vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, (int)ShaderTextureName::Count * max_count),
			vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, (int)ShaderTextureName::Count * max_count),
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

	void VulkanRenderState::UpdateGlobalDescriptorSet()
	{
		ShaderBindings common_bindings;
		auto* global_descriptor_set_data = current_shader->GetDescriptorSet(DescriptorSet::Global);
		if (global_descriptor_set_data->Empty())
			return;

		for (auto& binding : global_descriptor_set_data->bindings)
		{
			auto index = global_bindings.GetBindingIndex(binding.address.binding, binding.type);
			if (index == -1)
				throw std::runtime_error("Shader requires global binding that doesn't exist");

			if (binding.type == ShaderProgram::BindingType::Sampler)
			{
				auto& global_binding = global_bindings.GetTextureBindings()[index];
				// TODO: remove set
				common_bindings.AddTextureBinding(DescriptorSet::Global, global_binding.index, global_binding.texture);
			}
			else
			{
				auto& global_binding = global_bindings.GetBufferBindings()[index];
				// TODO: remove set
				common_bindings.AddBufferBinding(DescriptorSet::Global, global_binding.index, global_binding.offset, global_binding.size, global_binding.buffer);
			}
		}

		utils::SmallVector<uint32_t, 10> dynamic_offsets;
		for (auto& binding : common_bindings.GetBufferBindings())
		{
			dynamic_offsets.push_back(binding.offset);
		}

		auto descriptor_cache = Engine::GetVulkanContext()->GetDescriptorCache();
		auto descriptor_set = descriptor_cache->GetDescriptorSet(common_bindings, *global_descriptor_set_data);

		auto command_buffer = GetCurrentCommandBuffer()->GetCommandBuffer();
		command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, current_pipeline->GetPipelineLayout(), DescriptorSet::Global, 1u, &descriptor_set, dynamic_offsets.size(), dynamic_offsets.data());
		
		global_layout_hash = global_descriptor_set_data->layout_hash;
	}

	void VulkanRenderState::UpdateFrameDescriptorSets()
	{
		for (auto& set : frame_descriptor_sets)
			set = vk::DescriptorSet();

		auto& descriptor_set = descriptor_sets[DescriptorSet::Object];
		frame_descriptor_sets[DescriptorSet::Object] = GetDescriptorSet(descriptor_set, DescriptorSet::Object, current_shader);
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
			auto attachment_count = std::min(current_render_target->GetColorAttachmentCount() + current_render_target->HasDepth() ? 1u : 0u, (uint32_t)clear_values.size());

			vk::RenderPassBeginInfo render_pass_begin_info(
				current_render_pass->GetRenderPass(),
				current_render_target->GetFramebuffer(context->GetCurrentFrame()),
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
			if (!current_render_pass || !current_shader || !current_mesh)
				throw new std::runtime_error("RenderState not ready");

			VulkanPipelineInitializer initializer(current_shader, current_render_pass, current_mesh, &current_render_mode);
			current_pipeline = GetPipeline(initializer);

			auto command_buffer = GetCurrentCommandBuffer()->GetCommandBuffer();
			command_buffer.bindPipeline( vk::PipelineBindPoint::eGraphics, current_pipeline->GetPipeline());
		}

		if (dirty_flags & (int)DirtyFlags::DescriptorSet)
		{
			//UpdateFrameDescriptorSets();
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
			vk::Rect2D scissor(vk::Offset2D(current_scissor.x, current_scissor.y), vk::Extent2D(current_scissor.z, current_viewport.w));
			command_buffer.setScissor(0, 1, &scissor);
		}

		dirty_flags = 0;
	}

	void VulkanRenderState::SetClearValue(uint32_t index, vk::ClearValue value)
	{
		clear_values[index] = value;
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
			dirty_flags |= (int)DirtyFlags::Viewport;
			current_scissor = scissor;
		}
	}

	void VulkanRenderState::SetShader(const ShaderProgram& program)
	{
		if (current_shader != &program)
		{
			dirty_flags |= (int)DirtyFlags::Shader;
			current_shader = &program;

			auto* global_descriptor_set_data = program.GetDescriptorSet(DescriptorSet::Global);
			if (!global_descriptor_set_data->Empty() && global_layout_hash != global_descriptor_set_data->layout_hash)
			{
				dirty_flags |= (int)DirtyFlags::GlobalDescriptorSet;
			}
		}
	}

	vk::DescriptorSet VulkanRenderState::GetDescriptorSet(DescriptorSetData& set_data, const uint32_t set_index, const ShaderProgram* current_shader)
	{
		auto vk_context = Engine::Get()->GetContext();
		auto descriptor_cache = vk_context->GetDescriptorCache();
		return descriptor_cache->GetDescriptorSet(set_data.bindings, *current_shader->GetDescriptorSet(set_index));
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
			0.0f, true, 16, false);

		auto device = Engine::GetVulkanDevice();
		auto sampler = device.createSamplerUnique(sampler_info);
		auto result = sampler.get();
		sampler_cache[hash] = std::move(sampler);
		return result;
	}

	void VulkanRenderState::SetBindings(ShaderBindings& bindings)
	{
		dirty_flags |= (int)DirtyFlags::DescriptorSet;

		for (auto& set_data : descriptor_sets)
		{
			set_data.bindings.Clear();
			//memset(set_data.dynamic_offsets.data(), 0, sizeof(set_data.dynamic_offsets));
			set_data.dynamic_offsets.clear();
			set_data.active = false;
		}

		descriptor_sets[DescriptorSet::Object].active = true;

		for (auto& binding : bindings.GetBufferBindings())
		{
			assert(binding.set == DescriptorSet::Object);
			descriptor_sets[DescriptorSet::Object].bindings.AddBufferBinding(binding.set, binding.index, 0, binding.size, binding.buffer);
			//descriptor_sets[binding.set].dynamic_offsets.push_back(binding.offset);
		}

		auto sorted_bindings = bindings.GetBufferBindings();
		std::sort(sorted_bindings.begin(), sorted_bindings.end());
		for (int i = 0; i < sorted_bindings.size(); i++)
		{
			descriptor_sets[DescriptorSet::Object].dynamic_offsets.push_back(sorted_bindings[i].offset);
		}

		for (auto& binding : bindings.GetTextureBindings())
		{
			assert(binding.set == DescriptorSet::Object);
			descriptor_sets[DescriptorSet::Object].bindings.AddTextureBinding(DescriptorSet::Object, binding.index, binding.texture);
		}

	}

	void VulkanRenderState::SetGlobalBindings(const ShaderBindings& bindings)
	{
		global_bindings = bindings;
		global_layout_hash = 0;
		dirty_flags |= (int)DirtyFlags::GlobalDescriptorSet;
	}

	const std::vector<uint32_t>& VulkanRenderState::GetDynamicOffsets(uint32_t first_set, uint32_t last_set)
	{
		dynamic_offsets.clear();
		for (int set = first_set; set <= last_set; set++)
		{
			assert(frame_descriptor_sets[set]);
			for (int i = 0; i < descriptor_sets[set].buffer_bindings.size(); i++)
			{
				auto& buffer_binding = descriptor_sets[set].buffer_bindings[i];
				if (!buffer_binding.buffer)
					continue;

				dynamic_offsets.push_back(descriptor_sets[set].dynamic_offsets[i]);
			}
		}

		return dynamic_offsets;
	}

	void VulkanRenderState::BindFrameDescriptorSets(vk::CommandBuffer command_buffer, vk::PipelineBindPoint bind_point)
	{
		auto& dynamic_offsets = descriptor_sets[DescriptorSet::Object].dynamic_offsets;
		command_buffer.bindDescriptorSets(bind_point, current_pipeline->GetPipelineLayout(), DescriptorSet::Object, 1u, &frame_descriptor_sets[DescriptorSet::Object], dynamic_offsets.size(), dynamic_offsets.data());
	}

	void VulkanRenderState::RenderDrawCall(const ECS::components::DrawCall* draw_call, bool is_depth)
	{
		auto* mesh = draw_call->mesh;
		SetMesh(*mesh);

		auto* shader = is_depth ? draw_call->depth_only_shader : draw_call->shader;
		SetShader(*shader);
		// TODO: skinning offset

		UpdateState();

		if (!mesh->hasIndices())
		{
			throw std::runtime_error("mesh should have indices");
		}

		vk::DeviceSize offset = { 0 };
		vk::Buffer vertex_buffer = mesh->vertexBuffer()->Buffer();
		vk::Buffer index_buffer = mesh->indexBuffer()->Buffer();
		auto command_buffer = GetCurrentCommandBuffer()->GetCommandBuffer();
		command_buffer.bindVertexBuffers(0, 1, &vertex_buffer, &offset);
		command_buffer.bindIndexBuffer(index_buffer, offset, vk::IndexType::eUint16);

		utils::SmallVector<uint32_t, 4> dynamic_offsets;
		dynamic_offsets.push_back(draw_call->dynamic_offset);
		auto descriptor_set = is_depth ? draw_call->depth_only_descriptor_set : draw_call->descriptor_set;
		command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, current_pipeline->GetPipelineLayout(), DescriptorSet::Object, 1u, &draw_call->descriptor_set, dynamic_offsets.size(), dynamic_offsets.data());

		command_buffer.drawIndexed(static_cast<uint32_t>(mesh->indexCount()), 1, 0, 0, 0);
	}

	void VulkanRenderState::RenderDrawCall(const core::render::DrawCall* draw_call)
	{
		auto* mesh = draw_call->mesh;

		SetMesh(*mesh);
		SetShader(*draw_call->shader);
		SetBindings(*draw_call->shader_bindings);

		UpdateState();

		if (!mesh->hasIndices())
		{
			throw std::runtime_error("mesh should have indices");
		}

		vk::DeviceSize offset = { 0 };
		vk::Buffer vertex_buffer = mesh->vertexBuffer()->Buffer();
		vk::Buffer index_buffer = mesh->indexBuffer()->Buffer();
		auto command_buffer = GetCurrentCommandBuffer()->GetCommandBuffer();
		command_buffer.bindVertexBuffers(0, 1, &vertex_buffer, &offset);
		command_buffer.bindIndexBuffer(index_buffer, offset, vk::IndexType::eUint16);

		BindFrameDescriptorSets(command_buffer, vk::PipelineBindPoint::eGraphics);

		command_buffer.drawIndexed(static_cast<uint32_t>(mesh->indexCount()), 1, 0, 0, 0);
	}

	void VulkanRenderState::BeginRecording()
	{
		auto begin_info = vk::CommandBufferBeginInfo();
		auto command_buffer = GetCurrentCommandBuffer()->GetCommandBuffer();
		command_buffer.begin(begin_info);
	}

	VulkanCommandBuffer* VulkanRenderState::BeginRendering(const VulkanRenderTarget& render_target, const VulkanRenderPass& render_pass)
	{
		dirty_flags = (uint32_t)DirtyFlags::All;
		current_render_pass = nullptr;
		current_render_mode = RenderMode();
		current_mesh = nullptr;
		current_shader = nullptr;
		current_pipeline = nullptr;
		current_render_target = &render_target;
		render_pass_started = false;
		global_layout_hash = 0;

		for (auto& set_data : descriptor_sets)
			set_data.hash = 0;

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

	void VulkanRenderState::RecordCompute(const ShaderProgram& program, ShaderBindings& bindings, uvec3 group_size)
	{
		SetShader(program);
		VulkanPipelineInitializer compute_pipeline_initializer(&program);
		current_pipeline = GetPipeline(compute_pipeline_initializer);
		SetBindings(bindings);
		UpdateFrameDescriptorSets();

		auto command_buffer = GetCurrentCommandBuffer()->GetCommandBuffer();
		command_buffer.bindPipeline( vk::PipelineBindPoint::eCompute, current_pipeline->GetPipeline());
		BindFrameDescriptorSets(command_buffer, vk::PipelineBindPoint::eCompute);

		vkCmdDispatch(command_buffer, group_size.x, group_size.y, group_size.z);

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

} }