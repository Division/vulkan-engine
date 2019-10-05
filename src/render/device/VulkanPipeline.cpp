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

namespace core { namespace Device {

	VulkanPipelineInitializer::VulkanPipelineInitializer(const ShaderProgram* shader_program, const VulkanRenderPass* render_pass, const Mesh* mesh, const RenderMode* render_mode)
		: shader_program(shader_program)
		, render_pass(render_pass)
		, mesh(mesh)
		, render_mode(render_mode)
	{
		size_t hashes[] = { this->mesh->GetVertexAttribHash(), render_pass->GetHash(), shader_program->GetHash(), render_mode->GetHash() };
		hash = FastHash(hashes, sizeof(hashes));
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
		OutputDebugStringA((std::string("Pipeline state viewport: ") + std::to_string(viewport.width) + ", " + std::to_string(viewport.height) + "\n").c_str());
		vk::Rect2D scissor(vk::Offset2D(0, 0), context->GetSwapchain()->GetExtent());
		vk::PipelineViewportStateCreateInfo viewport_state({}, 1, &viewport, 1, &scissor);

		auto* render_mode = initializer.render_mode;
		vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info(
			{}, VK_FALSE, VK_FALSE, 
			vk::PolygonMode::eFill, 
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

		std::vector<vk::DescriptorSetLayout> descriptor_set_layouts;
		for (auto& set : shader_program->GetDescriptorSets())
		{
			if (set.Empty()) continue;
			descriptor_set_layouts.push_back(set.layout.get());
		}

		vk::PipelineLayoutCreateInfo pipeline_layout_info({}, descriptor_set_layouts.size(), descriptor_set_layouts.data());
		pipeline_layout = device.createPipelineLayoutUnique(pipeline_layout_info);

		// todo: Add stencil if required
		vk::PipelineDepthStencilStateCreateInfo pipeline_depth_stencil_info({}, render_mode->GetDepthTestEnabled(), render_mode->GetDepthWriteEnabled(), (vk::CompareOp)render_mode->GetDepthFunc());

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
			nullptr, 
			pipeline_layout.get(),
			render_pass->GetRenderPass()
		);

		pipeline = device.createGraphicsPipelineUnique(vk::PipelineCache(), pipeline_info);
	}

} }