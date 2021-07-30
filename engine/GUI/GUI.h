#pragma once

#define RMLUI_STATIC_LIB
#include <RmlUi/Core.h>
#include "Engine.h"

namespace GUI
{
	class SystemInterface : public Rml::SystemInterface
	{
	public:
		SystemInterface()
		{
			last_time = Engine::Get()->time();
		}

		double GetElapsedTime() override
		{
			return Engine::Get()->time() - last_time;
		}

		void Update()
		{
			last_time = Engine::Get()->time();
		}

	private:
		double last_time;
	};

	class RenderInterface : public Rml::RenderInterface
	{
		void RenderGeometry(Rml::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rml::TextureHandle texture, const Rml::Vector2f& translation) override {}
		void EnableScissorRegion(bool enable) override {}
		void SetScissorRegion(int x, int y, int width, int height) override {}
	};

	void Update(double dt);

	bool Initialize();

	void Deinitialize();

}