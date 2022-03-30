#pragma once

#include "CommonIncludes.h"
#include "VulkanCaps.h"
#include "render/shader/Shader.h"
#include "render/shader/ShaderBindings.h"
#include "render/renderer/IRenderer.h"

class Mesh;

namespace ECS
{
	namespace components
	{
		struct DrawCall;
	}
}

class VertexLayout;


namespace Device {

	class ShaderProgram;
	class VulkanPipeline;
	struct VulkanPipelineInitializer;
	class VulkanCommandPool;
	class VulkanCommandBuffer;
	class VulkanRenderPass;
	class VulkanRenderTarget;
	class Texture;

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

	enum class PolygonMode : int
	{
		Fill = VK_POLYGON_MODE_FILL,
		Line = VK_POLYGON_MODE_LINE,
		Point = VK_POLYGON_MODE_POINT
	};

	enum class PrimitiveTopology : int
	{
		PointList = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
		LineList = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
		LineStrip = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
		TriangleList = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		TriangleStrip = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
		TriangleFan = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
		LineListWithAdjacency = VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,
		LineStripWithAdjacency = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY,
		TriangleListWithAdjacency = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY,
		TriangleStripWithAdjacency = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY,
		PatchList = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST
	};

	enum class CullMode
	{
		None,
		Front,
		Back,
		FrontAndBack
	};

	struct SamplerMode
	{
		ShaderSamplerName name;
		uint32_t GetHash() const { return (uint32_t)name; }
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
		void SetPolygonMode(PolygonMode polygon_mode);
		void SetPrimitiveTopology(PrimitiveTopology topology);

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
		PolygonMode GetPolygonMode() const { return polygon_mode; };
		PrimitiveTopology GetPrimitiveTopology() const { return primitive_topology; };

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
		PolygonMode polygon_mode = PolygonMode::Fill;
		PrimitiveTopology primitive_topology = PrimitiveTopology::TriangleList;

		mutable bool hash_dirty = true;
		mutable uint32_t hash = 0;
	};

	enum class IndexType
	{
		UINT16 = VK_INDEX_TYPE_UINT16,
		UINT32 = VK_INDEX_TYPE_UINT32,
	};

	class DescriptorSet
	{
		const ShaderProgram::DescriptorSetLayout* layout;
		DescriptorSetBindings bindings;
		vk::DescriptorSet vulkan_descriptor_set;

	public:
		DescriptorSet(const DescriptorSetBindings& bindings, const ShaderProgram::DescriptorSetLayout* layout, vk::DescriptorSet vk_descriptor_set)
			: layout(layout), bindings(bindings), vulkan_descriptor_set(vk_descriptor_set) {}

		vk::DescriptorSet GetVKDescriptorSet() const { return vulkan_descriptor_set; }
		const DescriptorSetBindings& GetBindings() const { return bindings; };
		const ShaderProgram::DescriptorSetLayout& GetLayout() const { return *layout; }
	};

	class VulkanRenderState : NonCopyable
	{
	public:

		enum class DirtyFlags : uint32_t
		{
			Viewport = 1 << 0,
			VertexLayout = 1 << 1,
			RenderMode = 1 << 2,
			Shader = 1 << 3,
			RenderPass = 1 << 4,
			GlobalDescriptorSet = 1 << 5,
			Scissor = 1 << 7,
			All = ~0u
		};

		VulkanRenderState();
		~VulkanRenderState();

		void SetRenderMode(const RenderMode& mode);
		void SetViewport(vec4 viewport);
		vec4 GetViewport() const { return current_viewport; };
		void SetScissor(vec4 scissor);
		void SetShader(const ShaderProgram& program);
		void SetVertexLayout(const VertexLayout& layout);
		void SetGlobalBindings(const ResourceBindings& global_bindings, const ConstantBindings& global_constants = {});
		void SetDescriptorSetBindings(const DescriptorSetBindings& bindings, const ConstantBindings& constant_bindings = {});
		void SetDescriptorSet(const DescriptorSet& descriptor_set, uint32_t index, uint32_t dynamic_offset_count, const uint32_t* dynamic_offsets);
		void RemoveGlobalBindings();
		void SetClearValue(uint32_t index, vk::ClearValue value);
		void PushConstants(ShaderProgram::Stage stage, uint32_t offset, uint32_t size, void* data);
		const VulkanPipeline* GetCurrentPipeline() const { return current_pipeline; }

		void UpdateState();
		void RenderDrawCall(const ECS::components::DrawCall* draw_call, bool is_depth);
		void DrawIndexed(const VulkanBuffer& vertex_buffer, const VulkanBuffer& index_buffer, uint32_t vertex_offset, uint32_t index_count, uint32_t first_index, IndexType index_type, uint32_t instance_count = 1);
		void DrawIndexedIndirect(const VulkanBuffer& vertex_buffer, const VulkanBuffer& index_buffer, IndexType index_type, VulkanBuffer& indirect_buffer, uint32_t indirect_buffer_offset);
		void Draw(const VulkanBuffer& buffer, uint32_t vertexCount, uint32_t firstVertex, uint32_t instance_count = 1);
		void DrawIndirect(const VulkanBuffer& buffer, VulkanBuffer& indirect_buffer, uint32_t indirect_buffer_offset);

		VulkanCommandBuffer* GetCurrentCommandBuffer() const;
		void UpdateGlobalDescriptorSet();

		void BeginRenderPass(const Device::VulkanRenderPass& render_pass);
		void EndRenderPass();
		VulkanCommandBuffer* BeginRendering(const VulkanRenderTarget& render_target, const VulkanRenderPass& render_pass);
		void EndRendering();

		void Dispatch(const ShaderProgram& program, ResourceBindings& bindings, ConstantBindings& constants, uvec3 group_size);
		void DispatchIndirect(const ShaderProgram& program, ResourceBindings& bindings, ConstantBindings& constants, VulkanBuffer& buffer, uint32_t offset);

		void BeginRecording(PipelineBindPoint bind_point);
		void EndRecording();

	private:
		VulkanPipeline* GetPipeline(const VulkanPipelineInitializer& initializer);
		vk::Sampler GetSampler(const SamplerMode& sampler_mode);

		std::optional<ResourceBindings> global_bindings;
		std::optional<ConstantBindings> global_constants;

		uint32_t global_layout_hash = 0;
		PipelineBindPoint pipeline_bind_point = PipelineBindPoint::Graphics;

		uint32_t dirty_flags = 0;
		uint32_t current_frame = 0;
		bool render_pass_started = false;

		RenderMode current_render_mode;
		const VertexLayout* current_vertex_layout = nullptr;
		const VulkanRenderTarget* current_render_target = nullptr;
		const VulkanRenderPass* current_render_pass = nullptr;
		const ShaderProgram* current_shader = nullptr;
		const VulkanPipeline* current_pipeline = nullptr;
		vec4 current_viewport;
		vec4 current_scissor;

		std::unordered_map<uint32_t, vk::UniqueSampler> sampler_cache;
		std::unordered_map<uint32_t, vk::DescriptorSet> descriptor_set_cache;
		std::unordered_map<uint32_t, std::unique_ptr<VulkanPipeline>> pipeline_cache;
		std::array<vk::ClearValue, caps::max_color_attachments + 1> clear_values;

		std::unordered_map<uint32_t, std::unique_ptr<VulkanCommandPool>> command_pools;
		vk::UniqueDescriptorPool descriptor_pool;
		std::unordered_map<uint32_t, std::array<VulkanCommandBuffer*, caps::MAX_FRAMES_IN_FLIGHT>> command_buffers;
	};

}