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
	struct DebugSettings;
	class SceneRenderer;
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

	GLFWwindow* GetWindow() const { return window; }
	Device::VulkanContext* GetContext() const { return vulkan_context.get(); }
	Scene* GetScene() const { return scene.get(); }
	System::Input* GetInput() const { return input.get(); }
	render::DebugDraw* GetDebugDraw() const { return debug_draw.get(); }
	render::SceneRenderer* GetSceneRenderer() const { return scene_renderer.get(); }
	Device::ShaderCache* GetShaderCache() const { return shader_cache.get(); }
	IGame* GetGame() { return game.get(); }

	vec2 GetScreenSize();

	void MainLoop();

private:
	void WindowResize(int32_t width, int32_t height);
	static void SizeCallback(GLFWwindow* window, int32_t width, int32_t height);

private:
	static Engine* instance;
	std::unique_ptr<render::DebugSettings> debug_settings;
	std::unique_ptr<IGame> game;
	GLFWwindow* window;
	std::unique_ptr<Device::ShaderCache> shader_cache;
	std::unique_ptr<Device::VulkanContext> vulkan_context;
	std::unique_ptr<Scene> scene;
	std::unique_ptr<render::SceneRenderer> scene_renderer;
	std::unique_ptr<render::DebugDraw> debug_draw;
	std::unique_ptr<System::Input> input;

	bool loop_started = false;
	double last_time = 0;
	double current_time = 0;
};
