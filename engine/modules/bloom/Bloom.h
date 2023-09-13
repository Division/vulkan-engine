#pragma once

#include "render/shader/Shader.h"
#include "render/renderer/SceneRenderer.h"

namespace render::graph
{
	class RenderGraph;
	class IRenderPassBuilder;
	struct DependencyNode;
	struct ResourceWrapper;
}

namespace Device
{
	class VulkanRenderTargetAttachment;
	class Texture;
	class ShaderCache;
	class ResourceBindings;
	class ConstantBindings;
	class VulkanPipeline;
}

class Mesh;

namespace Modules
{
	struct EnvironmentSettings;
	struct RendererResources;

	struct BloomSettings
	{
		float bloom_threshold = 0.42;
		float bloom_strength = 0.14;
		float bloom_scatter = 0.6305;
		float bloom_clamp_max = 25000;
		bool bloom_enabled = true;
	};

	class Bloom
	{
	public:
		Bloom(const char* shaderDir, BloomSettings* bloomSettings);

		// To be passed into rpsProgramBindNodeSubprogram.
		// Add this node to your rpsl:
		// graphics node Bloom(rtv backbuffer : SV_Target0, srv srcTexture, float bloomThreshold, float bloomStrength, float scatter, float clampMax, float thresholdKnee);
		RpsSubprogram GetSubprogram() const { return *program; }

	private:
		void RenderCommon(const RpsCmdCallbackContext* pContext, Device::VulkanPipeline& pipeline, vk::ImageView srcTextureLow = nullptr, vec4 lowTexSize = vec4(0));
		void RenderPrefilter(const RpsCmdCallbackContext* pContext);
		void RenderBlurH(const RpsCmdCallbackContext* pContext);
		void RenderBlurV(const RpsCmdCallbackContext* pContext);
		void RenderUpsample(const RpsCmdCallbackContext* pContext);
		//graph::DependencyNode* AddPass(const char* name, Device::ShaderProgram& shader, graph::RenderGraph& graph, graph::DependencyNode& src_target_node, graph::DependencyNode* src_low_res_node, graph::ResourceWrapper& destination_target);

	private:

		Mesh* full_screen_quad_mesh;
		Device::ShaderProgram* shader_prefilter;
		Device::ShaderProgram* shader_blur_h;
		Device::ShaderProgram* shader_blur_v;
		Device::ShaderProgram* shader_upsample;

		render::RpsSubprogramHandle program;

		BloomSettings& bloom_settings;
	};

}