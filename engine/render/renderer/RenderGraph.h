#pragma once

#include "CommonIncludes.h"
#include "render/device/Types.h"

namespace core
{
	namespace Device
	{
		class Texture;
		class VulkanBuffer;
		class VulkanRenderTarget;
		class VulkanRenderPass;
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

		ImageLayout image_layout = ImageLayout::Undefined;

	};

	struct Pass;

	struct DependencyNode
	{
		ResourceWrapper* resource = nullptr;
		Pass* render_pass = nullptr;
		uint32_t index = -1;
		uint32_t order = 0;
		uint32_t group; // to discard all nodes not connected to present node
		bool on_stack = false;
		bool visited = false;
	};

	class IRenderPassBuilder;

	struct Pass
	{
		//typedef std::function<void(IRenderPassBuilder& builder)> InitCallback;
		typedef std::function<void()> RecordCallback;

		Pass() = default;
		Pass(Pass&&) = default;
		Pass& operator=(Pass&&) = default;
		Pass(const char* name, /*Pass::InitCallback init_callback,*/ Pass::RecordCallback record_callback)
			: name(name), /*init_callback(init_callback),*/ record_callback(record_callback) {}

		//InitCallback init_callback;
		RecordCallback record_callback;

		int order = -1;

		const char* name;
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

		template<typename T>
		T AddPass(char* name, std::function<T(IRenderPassBuilder& builder)> init_callback, Pass::RecordCallback record_callback)
		{
			render_passes.push_back(std::make_unique<Pass>(name, record_callback));
			current_render_pass = render_passes.back().get();
			return init_callback(*this);
		}

		void Clear();

		void Prepare();
		const std::vector<std::unique_ptr<Pass>>& GetRenderPasses() const { return render_passes; }

		// IRenderPassBuilder
		void AddInput(DependencyNode& node) override;
		DependencyNode* AddOutput(ResourceWrapper& render_target) override;
	private:
		bool ResourceRegistered(void* resource);

	private:
		Pass* current_render_pass = nullptr;
		DependencyNode* present_node = nullptr;
		std::vector<std::unique_ptr<Pass>> render_passes;
		std::vector<std::unique_ptr<ResourceWrapper>> resources;
		std::vector<std::unique_ptr<DependencyNode>> nodes;
		std::unordered_map<uint32_t, std::unique_ptr<VulkanRenderPass>> render_pass_cache;
	};

} } }
