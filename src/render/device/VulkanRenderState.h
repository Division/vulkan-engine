#pragma once

#include "CommonIncludes.h"
#include "VulkanCaps.h"

class Mesh;

namespace core { namespace Device {

	class ShaderProgram;
	class VulkanPipeline;
	struct VulkanPipelineInitializer;
	class VulkanCommandPool;
	class VulkanCommandBuffer;
	class VulkanRenderPass;
	class VulkanRenderTarget;

	enum class BlendFactor : int
	{
		Zero = VK_BLEND_FACTOR_ZERO,
		One = VK_BLEND_FACTOR_ONE,
		SrcColor = VK_BLEND_FACTOR_SRC_COLOR,
		OneMinusSrcColor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
		DstColor = VK_BLEND_FACTOR_DST_COLOR,
		OneMinusDstColor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
		SrcAlpha = VK_BLEND_FACTOR_SRC_ALPHA,
		OneMinusSrcAlpha = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		DstAlpha = VK_BLEND_FACTOR_DST_ALPHA,
		OneMinusDstAlpha = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
		ConstantColor = VK_BLEND_FACTOR_CONSTANT_COLOR,
		OneMinusConstantColor = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
		ConstantAlpha = VK_BLEND_FACTOR_CONSTANT_ALPHA,
		OneMinusConstantAlpha = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
		SrcAlphaSaturate = VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,
		Src1Color = VK_BLEND_FACTOR_SRC1_COLOR,
		OneMinusSrc1Color = VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
		Src1Alpha = VK_BLEND_FACTOR_SRC1_ALPHA,
		OneMinusSrc1Alpha = VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA
	};

	enum class BlendOp : int
	{
		Add = VK_BLEND_OP_ADD,
		Subtract = VK_BLEND_OP_SUBTRACT,
		ReverseSubtract = VK_BLEND_OP_REVERSE_SUBTRACT,
		Min = VK_BLEND_OP_MIN,
		Max = VK_BLEND_OP_MAX
	};

	enum class CompareOp : int
	{
		Never = VK_COMPARE_OP_NEVER,
		Less = VK_COMPARE_OP_LESS,
		Equal = VK_COMPARE_OP_EQUAL,
		LessOrEqual = VK_COMPARE_OP_LESS_OR_EQUAL,
		Greater = VK_COMPARE_OP_GREATER,
		NotEqual = VK_COMPARE_OP_NOT_EQUAL,
		GreaterOrEqual = VK_COMPARE_OP_GREATER_OR_EQUAL,
		Always = VK_COMPARE_OP_ALWAYS
	};

	enum class CullMode
	{
		None,
		Front,
		Back,
		FrontAndBack
	};


	class RenderMode
	{
	public:
		void SetCullMode(CullMode cull_mode);
		void SetDepthTestEnabled(bool enabled);
		void SetDepthWriteEnabled(bool enabled);
		void SetAlphaBlendEnabled(bool enabled);
		void SetDepthFunc(CompareOp compare_mode);
		void SetAlphaFunc(CompareOp compare_mode);
		void SetBlend(BlendOp blend_op);
		void SetBlendAlpha(BlendOp blend_op);
		void SetSrcBlend(BlendFactor blend_mode);
		void SetDestBlend(BlendFactor blend_mode);
		void SetSrcBlendAlpha(BlendFactor blend_mode);
		void SetDestBlendAlpha(BlendFactor blend_mode);

		CullMode GetCullMode() const { return cull_mode; };
		bool GetDepthTestEnabled() const { return depth_test_enabled; };
		bool GetDepthWriteEnabled() const { return depth_write_enabled; };
		bool GetAlphaBlendEnabled() const { return alpha_blend_enabled; };
		CompareOp GetDepthFunc() const { return depth_func; };
		CompareOp GetAlphaFunc() const { return alpha_func; };
		BlendOp GetBlend() const { return blend; };
		BlendOp GetBlendAlpha() const { return blend_alpha; };
		BlendFactor GetSrcBlend() const { return src_blend; };
		BlendFactor GetDestBlend() const { return dst_blend; };
		BlendFactor GetSrcBlendAlpha() const { return src_blend_alpha; };
		BlendFactor GetDestBlendAlpha() const { return dst_blend_alpha; };

		uint32_t GetHash() const;

	private:
		CullMode cull_mode = CullMode::Back;
		bool depth_test_enabled = false;
		bool depth_write_enabled = false;
		bool alpha_blend_enabled = false;
		CompareOp depth_func = CompareOp::Always;
		CompareOp alpha_func = CompareOp::Always;
		BlendOp blend = BlendOp::Add;
		BlendOp blend_alpha = BlendOp::Add;
		BlendFactor src_blend = BlendFactor::One;
		BlendFactor dst_blend = BlendFactor::One;
		BlendFactor src_blend_alpha = BlendFactor::One;
		BlendFactor dst_blend_alpha = BlendFactor::One;
		
		mutable bool hash_dirty = true;
		mutable uint32_t hash = 0;
	};

	class VulkanRenderState : NonCopyable
	{
	public:

		enum class DirtyFlags : uint32_t
		{
			Viewport = 1 << 0,
			Mesh = 1 << 1,
			RenderMode = 1 << 2,
			Shader = 1 << 3,
			RenderPass = 1 << 4,
			All = ~0u
		};

		VulkanRenderState();
		~VulkanRenderState();

		void SetRenderMode(const RenderMode& mode);
		void SetRenderPass(const VulkanRenderPass& render_pass);
		void SetViewport(vec4 viewport);
		void SetShader(const ShaderProgram& program);
		void RenderMesh(const Mesh& mesh);
		VulkanCommandBuffer* GetCurrentCommandBuffer() const { return command_buffers[current_frame]; }

		VulkanCommandBuffer* BeginRendering(const VulkanRenderTarget& render_target);
		void EndRendering();
		
	private:
		void UpdateState();
		void SetMesh(const Mesh& mesh);
		VulkanPipeline* GetPipeline(const VulkanPipelineInitializer& initializer);

		uint32_t dirty_flags = 0;
		uint32_t current_frame = 0;

		RenderMode current_render_mode;
		const Mesh* current_mesh = nullptr;
		const VulkanRenderTarget* current_render_target = nullptr;
		const VulkanRenderPass* current_render_pass = nullptr;
		const ShaderProgram* current_shader = nullptr;
		const VulkanPipeline* current_pipeline = nullptr;

		std::unordered_map<uint32_t, std::unique_ptr<VulkanPipeline>> pipeline_cache;
		std::unique_ptr<VulkanCommandPool> command_pool;
		std::array<VulkanCommandBuffer*, caps::MAX_FRAMES_IN_FLIGHT> command_buffers;
	};

} }