#include "GUI.h"

namespace GUI
{
	namespace
	{
		std::unique_ptr<SystemInterface> system_interface;
		std::unique_ptr<RenderInterface> render_interface;
		Rml::Context* default_context;
	}

	void Update(double dt)
	{
		system_interface->Update();
	}

	bool Initialize()
	{
		system_interface = std::make_unique<SystemInterface>();
		render_interface = std::make_unique<RenderInterface>();
		Rml::SetSystemInterface(system_interface.get());
		Rml::SetRenderInterface(render_interface.get());

		const auto screen_size = Engine::Get()->GetScreenSize();
		default_context = Rml::CreateContext("default", Rml::Vector2i(screen_size.x, screen_size.y));

		return true;
	}

	void Deinitialize()
	{
		system_interface = nullptr;
		render_interface = nullptr;
	}

}
