#pragma once

#include "CommonIncludes.h"

class Mesh;

namespace core { namespace Device {

	class ShaderProgram;
	class VulkanRenderPass;

	struct VulkanPipelineInitializer
	{
		VulkanPipelineInitializer(const ShaderProgram* shader_program, const VulkanRenderPass* render_pass, const Mesh* mesh);

		const ShaderProgram* shader_program;
		const VulkanRenderPass* render_pass;
		const Mesh* mesh;
	};

	class VulkanPipeline
	{
	public:
		VulkanPipeline(VulkanPipelineInitializer initializer);

		vk::Pipeline GetPipeline() const { return pipeline.get(); }
		vk::PipelineLayout GetPipelineLayout() const { return pipeline_layout.get(); }

	private:
		vk::UniquePipelineLayout pipeline_layout;
		vk::UniquePipeline pipeline;
	};

} }