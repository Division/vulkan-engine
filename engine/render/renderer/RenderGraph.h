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
		class VulkanSwapchain;
		class VulkanRenderTargetAttachment;
		class VulkanRenderState;
		struct VulkanRenderPassInitializer;
		struct VulkanRenderTargetInitializer;
	}
}


namespace core { namespace render { namespace graph {

	using namespace core::Device;

	enum class ResourceType : int
	{
		None,
		Attachment,
		Buffer
	};

	enum class InputUsage : int
	{
		Default,
		DepthAttachment
	};

	enum class PassQueue
	{
		Graphics,
		Compute,
		None
	};

	struct ResourceWrapper
	{
		enum class LastOperation : int
		{
			None,
			Write,
			Read
		};

		ResourceType type = ResourceType::None;
		void* resource_pointer = nullptr;

		ResourceWrapper() = default;
		ResourceWrapper(ResourceType type, void* pointer) : type(type), resource_pointer(pointer) {}


		VulkanBuffer* GetBuffer() const
		{
			if (type != ResourceType::Buffer) return nullptr;
			return reinterpret_cast<VulkanBuffer*>(resource_pointer);
		}

		VulkanRenderTargetAttachment* GetAttachment() const
		{
			if (type != ResourceType::Attachment) return nullptr;
			return reinterpret_cast<VulkanRenderTargetAttachment*>(resource_pointer);
		}

		PassQueue ownership_queue = PassQueue::None;
		LastOperation last_operation = LastOperation::None;
		ImageLayout image_layout = ImageLayout::Undefined;
		vk::Semaphore semaphore; // signalled when write is done
		//vk::AccessFlags access_flags;
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

		vk::ClearValue clear_value;
		bool should_clear = false;

		DependencyNode* Clear(vec4 color)
		{
			clear_value = vk::ClearColorValue(*(std::array<float, 4>*)&color);
			should_clear = true;
			return this;
		}

		DependencyNode* Clear(float value)
		{
			clear_value = vk::ClearDepthStencilValue(value, 0.0f);
			should_clear = true;
			return this;
		}
	};

	class IRenderPassBuilder;

	struct Pass
	{
		typedef std::function<void(VulkanRenderState& state)> RecordCallback;

		Pass() = default;
		Pass(Pass&&) = default;
		Pass& operator=(Pass&&) = default;
		Pass(const char* name, Pass::RecordCallback record_callback)
			: name(name), record_callback(record_callback) {}

		RecordCallback record_callback;

		int order = -1;

		const char* name;

		bool is_compute = false;

		uvec3 compute_group_size;
		std::vector<std::pair<DependencyNode*, InputUsage>> input_nodes;
		std::vector<DependencyNode*> output_nodes;
	};

	class IRenderPassBuilder
	{
	public:
		virtual ~IRenderPassBuilder() {};

		virtual void SetCompute() = 0;
		virtual void AddInput(DependencyNode& node, InputUsage usage = InputUsage::Default) = 0;
		virtual DependencyNode* AddOutput(ResourceWrapper& resource) = 0;
	};

	class RenderGraph : public IRenderPassBuilder
	{
	public:
		ResourceWrapper* RegisterAttachment(VulkanRenderTargetAttachment& attachment);
		ResourceWrapper* RegisterBuffer(VulkanBuffer& buffer);

		template<typename T>
		T AddPass(char* name, std::function<T(IRenderPassBuilder& builder)> init_callback, Pass::RecordCallback record_callback)
		{
			render_passes.push_back(std::make_unique<Pass>(name, record_callback));
			current_render_pass = render_passes.back().get();
			return init_callback(*this);
		}

		void Clear();
		void ClearCache();

		void Prepare();
		void Render();

		// IRenderPassBuilder
		void SetCompute() override;
		void AddInput(DependencyNode& node, InputUsage usage = InputUsage::Default) override;
		DependencyNode* AddOutput(ResourceWrapper& render_target) override;
	private:
		bool ResourceRegistered(void* resource);
		void RecordGraphicsPass(Pass* pass);
		void RecordComputePass(Pass* pass);
		VulkanRenderPass* GetRenderPass(const VulkanRenderPassInitializer& initializer);
		VulkanRenderTarget* GetRenderTarget(const VulkanRenderTargetInitializer& initializer);
		void ApplyPreBarriers(Pass& pass, VulkanRenderState& state);
		void ApplyPostBarriers(Pass& pass, VulkanRenderState& state);
		void RecordCommandBuffers();

	private:
		vk::Semaphore GetSemaphore();
		void UpdateInFlightSemaphores();

	private:
		Pass* current_render_pass = nullptr;
		DependencyNode* present_node = nullptr;
		std::vector<std::unique_ptr<Pass>> render_passes;
		std::vector<std::unique_ptr<ResourceWrapper>> resources;
		std::vector<std::unique_ptr<DependencyNode>> nodes;
		std::vector<vk::UniqueSemaphore> semaphores;
		std::vector<vk::Semaphore> available_semaphores;
		std::vector<std::pair<int, vk::Semaphore>> in_flight_semaphores;
		std::unordered_map<uint32_t, std::unique_ptr<VulkanRenderPass>> render_pass_cache;
		std::unordered_map<uint32_t, std::unique_ptr<VulkanRenderTarget>> render_target_cache;
	};

} } }
