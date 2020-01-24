#include "DrawCallManager.h"
#include "render/shader/Shader.h"
#include "render/shader/ShaderDefines.h"
#include "render/shader/ShaderBindings.h"
#include "render/renderer/SceneRenderer.h"
#include "render/shader/ShaderCache.h"
#include "ecs/components/DrawCall.h"
#include "render/device/VulkanDescriptorCache.h"
#include "Engine.h"

namespace core { namespace render {

	using namespace ECS;

	DrawCallManager::~DrawCallManager() = default;

	DrawCallManager::DrawCallManager(SceneRenderer& scene_renderer)
		: scene_renderer(scene_renderer)
	{
		manager = std::make_unique<EntityManager>();
		shader_cache = scene_renderer.GetShaderCache();
		depth_only_fragment_shader_hash = ShaderCache::GetShaderPathHash(L"shaders/noop.frag");
	}

	std::pair<EntityID, components::DrawCall*> DrawCallManager::AddDrawCall(const Mesh& mesh, const Material& material)
	{
		auto* descriptor_cache = Engine::GetVulkanContext()->GetDescriptorCache();

		components::DrawCall* draw_call;
		EntityID entity;
		{
			std::lock_guard<std::mutex> lock(mutex);
			entity = manager->CreateEntity();
			draw_call = manager->AddComponent<components::DrawCall>(entity);
		}

		*draw_call = components::DrawCall();
		draw_call->mesh = &mesh;
		
		bool has_skinning = false; // TODO: proper check

		ShaderCapsSet depth_caps;
		if (has_skinning)
			depth_caps.addCap(ShaderCaps::Skinning);

		auto depth_vertex_name_hash = material.GetVertexShaderDepthOnlyNameHash();
		auto depth_fragment_hash = depth_only_fragment_shader_hash;
		auto depth_vertex_hash =  ShaderCache::GetCombinedHash(depth_vertex_name_hash, depth_caps);

		auto vertex_name_hash = material.GetVertexShaderNameHash();
		auto fragment_hash = ShaderCache::GetCombinedHash(material.GetFragmentShaderNameHash(), material.shaderCaps());
		auto vertex_hash =  ShaderCache::GetCombinedHash(vertex_name_hash, material.shaderCaps());

		draw_call->shader = shader_cache->GetShaderProgram(vertex_hash, fragment_hash);
		draw_call->depth_only_shader = shader_cache->GetShaderProgram(depth_vertex_hash, depth_fragment_hash);
		
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

	void DrawCallManager::RemoveDrawCall(EntityID entity)
	{
		manager->DestroyEntity(entity);
	}

} }