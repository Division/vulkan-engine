#include "DrawCallManager.h"
#include "render/shader/Shader.h"
#include "render/shader/ShaderDefines.h"
#include "render/shader/ShaderBindings.h"
#include "render/renderer/SceneRenderer.h"
#include "ecs/CommandBuffer.h"
#include "render/device/VulkanDescriptorCache.h"
#include "Engine.h"
#include "utils/Pool.h"
#include "render/material/Material.h"

namespace render {

	using namespace ECS;
	using namespace Device;

	ECS::components::DrawCall* DrawCallManager::Handle::AddDrawCall(const DrawCallInitializer& initializer)
	{
		if (!manager)
			throw std::runtime_error("Handle isn't initialized");

		auto data = manager->AddDrawCall(initializer);
		draw_calls.push_back(data.first);
		return data.second;
	}

	DrawCallManager::Handle::Handle(Handle&& other)
	{
		draw_calls = std::move(other.draw_calls);
		manager = other.manager;
		other.manager = nullptr;
	}

	DrawCallManager::Handle& DrawCallManager::Handle::operator=(Handle&& other)
	{
		if (manager)
			RemoveAllDrawCalls();

		draw_calls = std::move(other.draw_calls);
		manager = other.manager;
		other.manager = nullptr;

		return *this;
	}

	bool DrawCallManager::Handle::RemoveDrawCall(ECS::EntityID draw_call)
	{
		if (!manager)
			throw std::runtime_error("Handle isn't initialized");

		bool found = false;

		for (size_t i = 0; i < draw_calls.size(); i++)
			if (draw_calls[i] == draw_call)
			{
				found = true;

				for (int j = i; j < (int)draw_calls.size() - 1; j++)
					draw_calls[j] = draw_calls[(size_t)j + 1];
				draw_calls.pop_back();

				manager->RemoveDrawCall(draw_call);
				break;
			}

		return found;
	}

	DrawCallManager::Handle::~Handle()
	{
		if (manager)
			RemoveAllDrawCalls();
	}

	void DrawCallManager::Handle::Reset()
	{
		if (manager)
			RemoveAllDrawCalls();

		manager = nullptr;
	}

	void DrawCallManager::Handle::RemoveAllDrawCalls()
	{
		for (int i = draw_calls.size() - 1; i >= 0; i--)
		{
			manager->RemoveDrawCall(draw_calls[i]);
		}
	}
	
	DrawCallManager::~DrawCallManager() = default;

	DrawCallManager::DrawCallManager(SceneRenderer& scene_renderer)
		: scene_renderer(scene_renderer)
	{
		manager = std::make_unique<EntityManager>();
		command_buffer = std::make_unique<ECS::CommandBuffer>(*manager);
		shader_cache = scene_renderer.GetShaderCache();
		draw_call_list_pool = std::make_unique<utils::Pool<DrawCallList>>();
	}

	Device::ShaderProgramInfo AppendDrawCallDefines(const Device::ShaderProgramInfo& src_info, const DrawCallInitializer& initializer, bool depth_only)
	{
		OPTICK_EVENT();
		auto result = src_info;
		ShaderCapsSet caps;
		if (initializer.has_skinning)
			caps.addCap(ShaderCaps::Skinning);

		auto& layout = initializer.mesh.GetVertexLayout();
		if (layout.HasAttrib(VertexAttrib::Origin))
			caps.addCap(ShaderCaps::VertexOrigin);

		if (layout.HasAttrib(VertexAttrib::JointIndices) && layout.HasAttrib(VertexAttrib::JointWeights))
			caps.addCap(ShaderCaps::VertexTBN);

		if (depth_only)
			caps.addCap(ShaderCaps::DepthOnly);

		std::vector<ShaderProgramInfo::Macro> macro;
		ShaderCache::AppendCapsDefines(caps, macro);

		for (auto& m : macro)
			result.AddMacro(m);

		return result;
	}

	std::pair<EntityID, components::DrawCall*> DrawCallManager::AddDrawCall(const DrawCallInitializer& initializer)
	{
		OPTICK_EVENT();
		auto* descriptor_cache = Engine::GetVulkanContext()->GetDescriptorCache();

		components::DrawCall* draw_call;
		EntityID entity;
		{
			OPTICK_EVENT("INIT1");
			entity = manager->CreateEntity();

			if (initializer.has_skinning)
			{
				manager->AddComponent<components::SkinningData>(entity);
			}

			draw_call = manager->AddComponent<components::DrawCall>(entity);
			draw_call->mesh = &initializer.mesh;
		}

		auto& material_depth_shader_info = initializer.material.GetDepthOnlyShaderInfo();
		auto& material_shader_info = initializer.material.GetShaderInfo();
		auto depth_shader_info = AppendDrawCallDefines(material_depth_shader_info, initializer, true);
		auto shader_info = AppendDrawCallDefines(material_shader_info, initializer, false);

		draw_call->shader = shader_cache->GetShaderProgram(shader_info);
		draw_call->depth_only_shader = shader_cache->GetShaderProgram(depth_shader_info);
		
		assert(draw_call->shader);
		assert(draw_call->depth_only_shader);

		auto* depth_descriptor_set = draw_call->depth_only_shader->GetDescriptorSetLayout(DescriptorSetType::Object);
		auto* descriptor_set = draw_call->shader->GetDescriptorSetLayout(DescriptorSetType::Object);

		auto material_resource_bindings = initializer.material.GetResourceBindings();
		material_resource_bindings.Merge(scene_renderer.GetGlobalResourceBindings());

		const DescriptorSetBindings depth_bindings(material_resource_bindings, *depth_descriptor_set);
		const DescriptorSetBindings bindings(material_resource_bindings, *descriptor_set);
		
		draw_call->depth_only_descriptor_set = descriptor_cache->GetDescriptorSet(depth_bindings);
		draw_call->descriptor_set = descriptor_cache->GetDescriptorSet(bindings);
		draw_call->visible = true;
		draw_call->material = &initializer.material;

		return std::make_pair(entity, draw_call);
	}

	ChunkList::List DrawCallManager::GetDrawCallChunks() const
	{
		return manager->GetChunkListsWithComponent<components::DrawCall>();
	}

	ChunkList::List DrawCallManager::GetSkinningChunks() const
	{
		return manager->GetChunkListsWithComponents<components::DrawCall, components::SkinningData>();
	}

	void DrawCallManager::RemoveDrawCall(EntityID entity)
	{
		manager->DestroyEntity(entity);
	}

	void DrawCallManager::Update()
	{
		command_buffer->Flush();
	}

	DrawCallList* DrawCallManager::ObtaintDrawCallList()
	{
		auto list = draw_call_list_pool->Obtain();
		auto* result = list.get();
		obtained_draw_call_lists.push_back(std::move(list));
		
		result->Clear();
		return result;
	}

	void DrawCallManager::ReleaseDrawCallLists()
	{
		for (auto& list : obtained_draw_call_lists)
		{
			draw_call_list_pool->Release(std::move(list));
		}

		obtained_draw_call_lists.clear();
	}

}