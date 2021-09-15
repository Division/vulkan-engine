#pragma once

#include "CommonIncludes.h"
#include "render/debug/DebugUI.h"

namespace System {

	enum class Key : int {
		Down = 0,
		Up,
		Left,
		Right,
		Number_1,
		Number_2,
		A,
		D,
		W,
		S,
		E,
		Q,
		C,
		X,
		Z,
		V,
		B,
		F,
		R,
		T,
		G,
		Space,
		Esc,
		Equal,
		Tab,
		CtrlLeft,
		AltLeft,

		MouseLeft,
		MouseRight,
	};

	class Input {
	public:
		Input(GLFWwindow* window);
		int keyDown(Key key) const;
		int keyDown(int key) const 
		{ 
			if (render::DebugUI::WantCaptureKeyboard())
				return 0;

			return glfwGetKey(window, key); 
		}

		bool FirstKeyDown(Key key) { return down_state[(uint32_t)key]; }

		int mouseDown(int key) const { return glfwGetMouseButton(window, key) && !render::DebugUI::WantCaptureMouse(); }
		const vec2 mousePosition() const { return _mousePos; }
		const vec2 mouseDelta() const { return _mouseDelta; }
		void update();

	private:
		std::vector<bool> down_state;
		std::vector<bool> last_down_state;
		GLFWwindow* window;
		vec2 _mousePos;
		vec2 _prevMousePos;
		vec2 _mouseDelta;
	};

}