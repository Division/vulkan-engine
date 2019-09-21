#pragma once

#include "CommonIncludes.h"

class Mesh;

namespace core { namespace Device {

	class ShaderProgram;
	class VulkanRenderPass;
	class RenderMode;

	struct VulkanPipelineInitializer
	{
		VulkanPipelineInitializer(const ShaderProgram* shader_program, const VulkanRenderPass* render_pass, const Mesh* mesh, const RenderMode* render_mode);

		const ShaderProgram const* shader_program;
		const VulkanRenderPass const* render_pass;
		const Mesh const* mesh;
		const RenderMode const* render_mode;

		uint32_t GetHash() const { return hash; }

	private:
		uint32_t hash;
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