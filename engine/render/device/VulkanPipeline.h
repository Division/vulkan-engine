#pragma once

#include "CommonIncludes.h"

namespace core
{
	class VertexLayout;
}

namespace core { namespace Device {

	class ShaderProgram;
	class VulkanRenderPass;
	class RenderMode;

	struct VulkanPipelineInitializer
	{
		VulkanPipelineInitializer(const ShaderProgram* shader_program, const VulkanRenderPass* render_pass, const VertexLayout* layout, const RenderMode* render_mode); // graphics
		VulkanPipelineInitializer(const ShaderProgram* shader_program); // compute

		const ShaderProgram const* shader_program = nullptr;
		const VulkanRenderPass const* render_pass = nullptr;
		const VertexLayout const* layout = nullptr;
		const RenderMode const* render_mode = nullptr;
		bool is_compute = false;

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
		bool is_compute;
		vk::UniquePipelineLayout pipeline_layout;
		vk::UniquePipeline pipeline;
	};

} }