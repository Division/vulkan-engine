#pragma once

#include "CommonIncludes.h"

namespace core
{
	namespace Device
	{
		class Texture;
		class VulkanBuffer;
		class VulkanRenderTarget;
	}
}


namespace core { namespace render { namespace graph {

	using namespace core::Device;

	enum class ResourceType : int
	{
		None,
		RenderTarget,
		Buffer
	};

	enum class PassQueue
	{
		Graphics,
		Compute
	};

	struct ResourceWrapper
	{
		ResourceType type = ResourceType::None;
		void* resource_pointer = nullptr;

		ResourceWrapper() = default;
		ResourceWrapper(ResourceType type, void* pointer) : type(type), resource_pointer(pointer) {}

		VulkanBuffer* GetBuffer() const
		{
			if (type != ResourceType::Buffer) return nullptr;
			return reinterpret_cast<VulkanBuffer*>(resource_pointer);
		}

		VulkanRenderTarget* GetRenderTarget() const
		{
			if (type != ResourceType::RenderTarget) return nullptr;
			return reinterpret_cast<VulkanRenderTarget*>(resource_pointer);
		}

	};

	struct Pass;

	struct DependencyNode
	{
		ResourceWrapper* resource = nullptr;
		Pass* render_pass = nullptr;
		uint32_t index = -1;
		uint32_t order = 0;
		bool on_stack = false;
		bool visited = false;
	};

	class IRenderPassBuilder;

	struct Pass
	{
		typedef std::function<void(IRenderPassBuilder& builder)> InitCallback;
		typedef std::function<void()> RecordCallback;

		Pass() = default;
		Pass(Pass&&) = default;
		Pass(char* name, Pass::InitCallback init_callback, Pass::RecordCallback record_callback)
			: name(name), init_callback(init_callback), record_callback(record_callback) {}

		InitCallback init_callback;
		RecordCallback record_callback;

		int order = -1; // distance from present node. -1 for non-visited passes (to be skipped)

		char* name;
		std::vector<DependencyNode*> input_nodes;
		std::vector<DependencyNode*> output_nodes;
	};

	class IRenderPassBuilder
	{
	public:
		virtual ~IRenderPassBuilder() {};

		virtual void AddInput(DependencyNode& node) = 0;
		virtual DependencyNode* AddOutput(ResourceWrapper& resource) = 0;
	};

	class RenderGraph : public IRenderPassBuilder
	{
	public:
		ResourceWrapper* RegisterRenderTarget(VulkanRenderTarget& render_target);
		ResourceWrapper* RegisterBuffer(VulkanBuffer& buffer);

		void AddPass(char* name, Pass::InitCallback init_callback, Pass::RecordCallback record_callback);
		void Clear();

		void SetPresentNode(DependencyNode& resource);
		void Prepare();
		const std::vector<Pass>& GetRenderPasses() const { return render_passes; }

		// IRenderPassBuilder
		void AddInput(DependencyNode& node) override;
		DependencyNode* AddOutput(ResourceWrapper& render_target) override;
	private:
		bool ResourceRegistered(void* resource);

	private:
		Pass* current_render_pass = nullptr;
		DependencyNode* present_node = nullptr;
		std::vector<Pass> render_passes;
		std::vector<ResourceWrapper> resources;
		std::vector<DependencyNode> nodes;
	};

} } }