#pragma once

#include "CommonIncludes.h"
#include "IGame.h"

class Scene;

namespace ECS
{
	class EntityManager;
	class TransformGraph;
}

namespace Device 
{
	class VulkanContext;
	class ShaderCache;
}

namespace render
{
	class SceneRenderer;
	class MaterialManager;
	class DebugDraw;
}

namespace System
{
	class Input;
}

class Engine : public NonCopyable
{
public:
	static Engine* Get() { return instance; };
	static Device::VulkanContext* GetVulkanContext();
	static vk::Device GetVulkanDevice();

	Engine(std::unique_ptr<IGame> game);
	~Engine();
		
	double time() const { return current_time; }

	ECS::EntityManager* GetEntityManager() const;
	ECS::TransformGraph* GetTransformGraph() const;

	Device::VulkanContext* GetContext() const { return vulkan_context.get(); }
	Scene* GetScene() const { return scene.get(); }
	render::MaterialManager* GetMaterialManager() const { return material_manager.get(); }
	System::Input* GetInput() const { return input.get(); }
	render::DebugDraw* GetDebugDraw() const { return debug_draw.get(); }

	void MainLoop();

private:
	void WindowResize(int32_t width, int32_t height);
	static void SizeCallback(GLFWwindow* window, int32_t width, int32_t height);

private:
	static Engine* instance;
	std::unique_ptr<IGame> game;
	GLFWwindow* window;
	std::unique_ptr<Device::ShaderCache> shader_cache;
	std::unique_ptr<Device::VulkanContext> vulkan_context;
	std::unique_ptr<Scene> scene;
	std::unique_ptr<render::SceneRenderer> scene_renderer;
	std::unique_ptr<render::MaterialManager> material_manager;
	std::unique_ptr<render::DebugDraw> debug_draw;
	std::unique_ptr<System::Input> input;

	bool loop_started = false;
	double last_time = 0;
	double current_time = 0;
};
