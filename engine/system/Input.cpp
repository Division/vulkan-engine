#include "Input.h"
#include <magic_enum/magic_enum.hpp>

namespace System {

	const std::unordered_map<int, int> INPUT_CONVERSION = {
		{ (int)Key::Left, GLFW_KEY_LEFT },
		{ (int)Key::Right, GLFW_KEY_RIGHT },
		{ (int)Key::Down, GLFW_KEY_DOWN},
		{ (int)Key::Up, GLFW_KEY_UP },
		{ (int)Key::Number_1, GLFW_KEY_1 },
		{ (int)Key::Number_2, GLFW_KEY_2 },
		{ (int)Key::A, GLFW_KEY_A },
		{ (int)Key::W, GLFW_KEY_W },
		{ (int)Key::S, GLFW_KEY_S },
		{ (int)Key::D, GLFW_KEY_D },
		{ (int)Key::E, GLFW_KEY_E },
		{ (int)Key::Q, GLFW_KEY_Q },
		{ (int)Key::C, GLFW_KEY_C },
		{ (int)Key::X, GLFW_KEY_X },
		{ (int)Key::Z, GLFW_KEY_Z },
		{ (int)Key::V, GLFW_KEY_V },
		{ (int)Key::B, GLFW_KEY_B },
		{ (int)Key::F, GLFW_KEY_F },
		{ (int)Key::R, GLFW_KEY_R },
		{ (int)Key::T, GLFW_KEY_T },
		{ (int)Key::G, GLFW_KEY_G },
		{ (int)Key::Space, GLFW_KEY_SPACE },
		{ (int)Key::Esc, GLFW_KEY_ESCAPE },
		{ (int)Key::Equal, GLFW_KEY_EQUAL},
		{ (int)Key::Tab, GLFW_KEY_TAB},
		{ (int)Key::CtrlLeft, GLFW_KEY_LEFT_CONTROL},
		{ (int)Key::AltLeft, GLFW_KEY_LEFT_ALT},
		{ (int)Key::MouseLeft, GLFW_MOUSE_BUTTON_1 },
		{ (int)Key::MouseRight, GLFW_MOUSE_BUTTON_2 }
	};

	Input::Input(GLFWwindow* window) : window(window)
	{
		down_state.resize(magic_enum::enum_count<Key>());
		std::fill(down_state.begin(), down_state.end(), false);
		last_down_state.resize(magic_enum::enum_count<Key>());
		std::fill(last_down_state.begin(), last_down_state.end(), false);
	}

	int Input::keyDown(Key key) const {
		switch (key) {
		case Key::MouseLeft:
		case Key::MouseRight:
			return mouseDown(INPUT_CONVERSION.at((int)key));

		default:
			return keyDown(INPUT_CONVERSION.at((int)key));
		}
	}

	void Input::update() {
		double x, y;
		glfwGetCursorPos(window, &x, &y);
		_prevMousePos = _mousePos;
		_mousePos = vec2(x, y);
		_mouseDelta = _mousePos - _prevMousePos;

		for (uint32_t i = 0; i < down_state.size(); i++)
		{
			down_state[i] = !last_down_state[i] && keyDown((Key)i);
			last_down_state[i] = keyDown((Key)i);
		}
	}

}