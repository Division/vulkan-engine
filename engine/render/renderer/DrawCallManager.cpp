#include "DrawCallManager.h"
#include "render/shader/Shader.h"
#include "render/shader/ShaderDefines.h"
#include "render/shader/ShaderBindings.h"
#include "render/renderer/SceneRenderer.h"
#include "ecs/components/DrawCall.h"
#include "ecs/CommandBuffer.h"
#include "render/device/VulkanDescriptorCache.h"
#include "Engine.h"
#include "utils/Pool.h"
#include "render/material/Material.h"

namespace render {

	using namespace ECS;
	using namespace Device;

	ECS::components::DrawCall* DrawCallManager::Handle::AddDrawCall(const Mesh& mesh, const Material& material)
	{
		if (!manager)
			throw std::runtime_error("Handle isn't initialized");

		auto data = manager->AddDrawCall(mesh, material);
		draw_calls.push_back(data);
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
		RemoveAllDrawCalls();
		draw_calls = std::move(other.draw_calls);
		manager = other.manager;
		other.manager = nullptr;

		return *this;
	}

	bool DrawCallManager::Handle::RemoveDrawCall(ECS::components::DrawCall* draw_call)
	{
		if (!manager)
			throw std::runtime_error("Handle isn't initialized");

		bool found = false;

		for (int i = 0; i < draw_calls.size(); i++)
			if (draw_calls[i].second == draw_call)
			{
				found = true;
				manager->RemoveDrawCall(draw_calls[i].first);
				draw_calls[i] = draw_calls[draw_calls.size() - 1];
				draw_calls.pop_back();
				break;
			}

		return found;
	}

	DrawCallManager::Handle::~Handle()
	{
		if (manager)
			RemoveAllDrawCalls();
	}

	void DrawCallManager::Handle::RemoveAllDrawCalls()
	{
		for (auto& item : draw_calls)
		{
			manager->RemoveDrawCall(item.first);
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

	std::pair<EntityID, components::DrawCall*> DrawCallManager::AddDrawCall(const Mesh& mesh, const Material& material)
	{
		auto* descriptor_cache = Engine::GetVulkanContext()->GetDescriptorCache();

		components::DrawCall* draw_call;
		EntityID entity;
		entity = manager->CreateEntity();
		draw_call = manager->AddComponent<components::DrawCall>(entity);

		*draw_call = components::DrawCall();
		draw_call->mesh = &mesh;

		bool has_skinning = false; // TODO: proper check

		ShaderCapsSet depth_caps;
		if (has_skinning)
			depth_caps.addCap(ShaderCaps::Skinning);

		auto& depth_shader_info = material.GetDepthOnlyShaderInfo();
		auto& shader_info = material.GetShaderInfo();

		draw_call->shader = shader_cache->GetShaderProgram(shader_info);
		draw_call->depth_only_shader = shader_cache->GetShaderProgram(depth_shader_info);
		
		auto* depth_descriptor_set = draw_call->depth_only_shader->GetDescriptorSet(DescriptorSet::Object);
		auto* descriptor_set = draw_call->shader->GetDescriptorSet(DescriptorSet::Object);
		ShaderBindings depth_bindings;
		ShaderBindings bindings;
		scene_renderer.SetupShaderBindings(material, *depth_descriptor_set, depth_bindings);
		scene_renderer.SetupShaderBindings(material, *descriptor_set, bindings);
		
		draw_call->depth_only_descriptor_set = descriptor_cache->GetDescriptorSet(depth_bindings, *depth_descriptor_set);
		draw_call->descriptor_set = descriptor_cache->GetDescriptorSet(bindings, *descriptor_set);
		draw_call->visible = true;

		return std::make_pair(entity, draw_call);
	}

	ChunkList::List DrawCallManager::GetDrawCallChunks()
	{
		return manager->GetChunkListsWithComponent<components::DrawCall>();
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