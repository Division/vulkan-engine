#include "VulkanPipeline.h"
#include "render/shader/Shader.h"
#include "VulkanRenderPass.h"
#include "render/mesh/Mesh.h"
#include "render/shader/ShaderDefines.h"
#include "Engine.h"
#include "render/device/VulkanContext.h"
#include "render/device/VulkanSwapchain.h"
#include "VulkanRenderState.h"
#include "utils/Math.h"

namespace Device {

	VulkanPipelineInitializer::VulkanPipelineInitializer(const ShaderProgram* shader_program, const VulkanRenderPass* render_pass, const VertexLayout* layout, const RenderMode* render_mode)
		: shader_program(shader_program)
		, render_pass(render_pass)
		, layout(layout)
		, render_mode(render_mode)
	{
		size_t hashes[] = { layout->GetHash(), render_pass->GetHash(), shader_program->GetHash(), render_mode->GetHash() };
		hash = FastHash(hashes, sizeof(hashes));
	}

	VulkanPipelineInitializer::VulkanPipelineInitializer(const ShaderProgram* shader_program)
		: shader_program(shader_program)
	{
		size_t hashes[] = { shader_program->GetHash() };
		hash = FastHash(hashes, sizeof(hashes));
		is_compute = true;
	}

	VulkanPipeline::VulkanPipeline(VulkanPipelineInitializer initializer)
		: is_compute(initializer.is_compute)
	{
		auto* shader_program = initializer.shader_program;
		auto* render_pass = initializer.render_pass;
		auto* layout = initializer.layout;
		auto* context = Engine::GetVulkanContext();
		auto device = context->GetDevice();

		std::array<vk::PipelineShaderStageCreateInfo, 5> shader_stages;
		uint32_t shader_stage_count = 0;

		if (!initializer.is_compute)
			assert(shader_program->VertexModule());

		if (shader_program->VertexModule())
		{
			assert(!initializer.is_compute);
			vk::PipelineShaderStageCreateInfo vertex_shader_stage_info(
				{}, 
				vk::ShaderStageFlagBits::eVertex, 
				shader_program->VertexModule()->GetModule(),
				shader_program->GetEntryPoint(ShaderProgram::Stage::Vertex)
			);
			shader_stages[shader_stage_count++] = vertex_shader_stage_info;
		}

		if (shader_program->FragmentModule())
		{
			assert(!initializer.is_compute);
			vk::PipelineShaderStageCreateInfo fragment_shader_stage_info(
				{},
				vk::ShaderStageFlagBits::eFragment,
				shader_program->FragmentModule()->GetModule(),
				shader_program->GetEntryPoint(ShaderProgram::Stage::Fragment)
			);
			shader_stages[shader_stage_count++] = fragment_shader_stage_info;
		}

		if (shader_program->ComputeModule())
		{
			assert(initializer.is_compute);
			vk::PipelineShaderStageCreateInfo compute_shader_stage_info(
				{},
				vk::ShaderStageFlagBits::eCompute,
				shader_program->ComputeModule()->GetModule(),
				shader_program->GetEntryPoint(ShaderProgram::Stage::Compute)
			);
			shader_stages[shader_stage_count++] = compute_shader_stage_info;
		}

		std::vector<vk::DescriptorSetLayout> descriptor_set_layouts;
		for (auto& set : shader_program->GetDescriptorSets())
		{
			if (set.Empty()) continue;
			descriptor_set_layouts.push_back(set.layout.get());
		}

		auto* push_constants = shader_program->GetPushConstants();

		utils::SmallVector<vk::PushConstantRange, 2> push_constant_ranges;
		for (int i = 0; i < push_constants->size(); i++)
		{
			auto& item = (*push_constants)[i];
			push_constant_ranges.push_back(vk::PushConstantRange(vk::ShaderStageFlags(item.stage_flags), item.offset, item.range));
		}

		vk::PipelineLayoutCreateInfo pipeline_layout_info({}, descriptor_set_layouts.size(), descriptor_set_layouts.data(), push_constant_ranges.size(), push_constant_ranges.data());
		pipeline_layout = device.createPipelineLayoutUnique(pipeline_layout_info);

		if (initializer.is_compute)
		{
			assert(shader_stage_count == 1);
			vk::ComputePipelineCreateInfo compute_pipeline_info({}, shader_stages[0], pipeline_layout.get());
			pipeline = device.createComputePipelineUnique({}, compute_pipeline_info);
		} else
		{
			vk::VertexInputBindingDescription vertex_binding_description(0, layout->GetStride(), vk::VertexInputRate::eVertex);

			std::vector<vk::VertexInputAttributeDescription> vertex_attribute_descriptions;
			auto& vertex_attribs = shader_program->VertexModule()->GetReflectionInfo()->VertexAttribs();
			for (auto& attrib : vertex_attribs)
			{
				vertex_attribute_descriptions.push_back(
					vk::VertexInputAttributeDescription(
						attrib.location,
						0,
						vk::Format(layout->GetAttribFormat(attrib.vertex_attrib)),
						layout->GetAttribOffset(attrib.vertex_attrib)
					)
				);
			}

			auto* render_mode = initializer.render_mode;
			vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info({}, 1, &vertex_binding_description, vertex_attribute_descriptions.size(), vertex_attribute_descriptions.data());
			vk::PipelineInputAssemblyStateCreateInfo input_assembly_create_info({}, vk::PrimitiveTopology(render_mode->GetPrimitiveTopology()), VK_FALSE);
		
			vk::Viewport viewport(0, 0, 0, 0, 0, 1);
			vk::Rect2D scissor(vk::Offset2D(0, 0), vk::Extent2D(0, 0));
			vk::PipelineViewportStateCreateInfo viewport_state({}, 1, &viewport, 1, &scissor);

			vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info(
				{}, VK_FALSE, VK_FALSE,
				vk::PolygonMode(render_mode->GetPolygonMode()), 
				vk::CullModeFlagBits(render_mode->GetCullMode()),
				vk::FrontFace::eCounterClockwise, VK_FALSE, 0, 0, 0.0f, 1.0f
			);

			vk::PipelineMultisampleStateCreateInfo multisampling_state_create_info({}, vk::SampleCountFlagBits::e1, VK_FALSE, 0, nullptr, VK_FALSE, VK_FALSE);

			vk::PipelineColorBlendAttachmentState color_blend_attachment(
				vk::Bool32(render_mode->GetAlphaBlendEnabled()),
				vk::BlendFactor(render_mode->GetSrcBlend()), 
				vk::BlendFactor(render_mode->GetDestBlend()), 
				vk::BlendOp(render_mode->GetBlend()),
				vk::BlendFactor(render_mode->GetSrcBlendAlpha()), 
				vk::BlendFactor(render_mode->GetDestBlendAlpha()), 
				vk::BlendOp(render_mode->GetBlendAlpha())
			);
			color_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

			vk::PipelineColorBlendStateCreateInfo color_blending({}, VK_FALSE, vk::LogicOp::eCopy, 1, &color_blend_attachment);

			// todo: Add stencil if required
			vk::PipelineDepthStencilStateCreateInfo pipeline_depth_stencil_info({}, render_mode->GetDepthTestEnabled(), render_mode->GetDepthWriteEnabled(), (vk::CompareOp)render_mode->GetDepthFunc());

			std::array<vk::DynamicState, 2> dynamic_states = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
			vk::PipelineDynamicStateCreateInfo pipeline_dynamic_state_info({}, dynamic_states.size(), dynamic_states.data());

			vk::GraphicsPipelineCreateInfo pipeline_info(
				{},
				shader_stage_count, shader_stages.data(), 
				&vertex_input_state_create_info, 
				&input_assembly_create_info,
				nullptr,
				&viewport_state, 
				&rasterization_state_create_info, 
				&multisampling_state_create_info, 
				&pipeline_depth_stencil_info, 
				&color_blending,
				&pipeline_dynamic_state_info,
				pipeline_layout.get(),
				render_pass->GetRenderPass()
			);

			pipeline = device.createGraphicsPipelineUnique(vk::PipelineCache(), pipeline_info);
		}
	}

}