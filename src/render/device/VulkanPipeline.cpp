#include "VulkanPipeline.h"
#include "render/shader/Shader.h"
#include "VulkanRenderPass.h"
#include "render/mesh/Mesh.h"
#include "render/shader/ShaderDefines.h"
#include "Engine.h"
#include "render/device/VulkanContext.h"
#include "render/device/VulkanSwapchain.h"

namespace core { namespace Device {

	VulkanPipelineInitializer::VulkanPipelineInitializer(const ShaderProgram* shader_program, const VulkanRenderPass* render_pass, const Mesh* mesh)
		: shader_program(shader_program)
		, render_pass(render_pass)
		, mesh(mesh)
	{
		
	}

	VulkanPipeline::VulkanPipeline(VulkanPipelineInitializer initializer)
	{
		auto* shader_program = initializer.shader_program;
		auto* render_pass = initializer.render_pass;
		auto* mesh = initializer.mesh;
		auto* context = Engine::GetVulkanContext();
		auto device = context->GetDevice();

		std::array<vk::PipelineShaderStageCreateInfo, 5> shader_stages;
		uint32_t shader_stage_count = 0;

		vk::PipelineShaderStageCreateInfo vertex_shader_stage_info(
			{}, 
			vk::ShaderStageFlagBits::eVertex, 
			shader_program->VertexModule().GetModule(), 
			"main"
		);
		shader_stages[shader_stage_count++] = vertex_shader_stage_info;

		if (shader_program->FragmentModule().HasModule())
		{
			vk::PipelineShaderStageCreateInfo fragment_shader_stage_info(
				{},
				vk::ShaderStageFlagBits::eFragment,
				shader_program->FragmentModule().GetModule(),
				"main"
			);
			shader_stages[shader_stage_count++] = fragment_shader_stage_info;
		}

		vk::VertexInputBindingDescription vertex_binding_description(0, mesh->strideBytes(), vk::VertexInputRate::eVertex);

		std::vector<vk::VertexInputAttributeDescription> vertex_attribute_descriptions;
		auto& vertex_attribs = shader_program->VertexModule().GetReflectionInfo()->VertexAttribs();
		for (auto& attrib : vertex_attribs)
		{
			vertex_attribute_descriptions.push_back(
				vk::VertexInputAttributeDescription(
					attrib.location, 
					0, 
					VERTEX_ATTRIB_FORMATS.at(attrib.vertex_attrib), 
					mesh->attribOffsetBytes(attrib.vertex_attrib)
				)
			);
		}

		vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info({}, 1, &vertex_binding_description, vertex_attribute_descriptions.size(), vertex_attribute_descriptions.data());
		vk::PipelineInputAssemblyStateCreateInfo input_assembly_create_info({}, vk::PrimitiveTopology::eTriangleList, VK_FALSE);
		
		vk::Viewport viewport(0, 0, (float)context->GetSwapchain()->GetWidth(), (float)context->GetSwapchain()->GetHeight(), 0, 1);
		vk::Rect2D scissor(vk::Offset2D(0, 0), context->GetSwapchain()->GetExtent());
		vk::PipelineViewportStateCreateInfo viewport_state({}, 1, &viewport, 1, &scissor);

		vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info({}, VK_FALSE, VK_FALSE, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise, VK_FALSE, 0, 0, 0.0f, 1.0f);

		vk::PipelineMultisampleStateCreateInfo multisampling_state_create_info({}, vk::SampleCountFlagBits::e1, VK_FALSE, 0, nullptr, VK_FALSE, VK_FALSE);

		vk::PipelineColorBlendAttachmentState color_blend_attachment;
		color_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
		color_blend_attachment.blendEnable = VK_FALSE;

		vk::PipelineColorBlendStateCreateInfo color_blending({}, VK_FALSE, vk::LogicOp::eCopy, 1, &color_blend_attachment);

		vk::PipelineLayoutCreateInfo pipeline_layout_info({}, 1, &shader_program->GetDescriptorSetLayout());
		pipeline_layout = device.createPipelineLayoutUnique(pipeline_layout_info);

		vk::GraphicsPipelineCreateInfo pipeline_info(
			{},
			shader_stage_count, shader_stages.data(), 
			&vertex_input_state_create_info, 
			&input_assembly_create_info, 
			nullptr, 
			&viewport_state, 
			&rasterization_state_create_info, 
			&multisampling_state_create_info, 
			nullptr, 
			&color_blending, 
			nullptr, 
			pipeline_layout.get(),
			render_pass->GetRenderPass()
		);

		pipeline = device.createGraphicsPipelineUnique(vk::PipelineCache(), pipeline_info);
	}

} }