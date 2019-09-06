//
// Created by Sidorenko Nikita on 3/24/18.
//

#include "Input.h"
#include "Logging.h"
#include <unordered_map>
#include "EngineMath.h"
#include <windowsx.h>

const std::unordered_map<int, int> INPUT_CONVERSION = {
    { (int)Key::Left, VK_LEFT },
    { (int)Key::Right, VK_RIGHT },
    { (int)Key::Down, VK_DOWN},
    { (int)Key::Up, VK_UP },
    { (int)Key::A, 'A' },
    { (int)Key::W, 'W' },
    { (int)Key::S, 'S' },
    { (int)Key::D, 'D' },
    { (int)Key::E, 'E' },
    { (int)Key::Q, 'Q' },
    { (int)Key::C, 'C' },
    { (int)Key::X, 'X' },
    { (int)Key::Z, 'Z' },
    { (int)Key::V, 'V' },
    { (int)Key::B, 'B' },
    { (int)Key::F, 'F' },
    { (int)Key::R, 'R' },
    { (int)Key::T, 'T' },
    { (int)Key::G, 'G' },
    { (int)Key::Space, VK_SPACE },
    { (int)Key::Esc, VK_ESCAPE },
    { (int)Key::Equal, 187 },
    { (int)Key::Tab, VK_TAB},
    { (int)Key::MouseLeft, 0 },
    { (int)Key::MouseRight, 1 }
};

int Input::keyDown(Key key) const {
	return _keyState[INPUT_CONVERSION.at((int)key)];
}

void Input::_update() {
	_mouseDelta = _mousePos - _prevMousePos;
	_prevMousePos = _mousePos;
}

void Input::_processMouseEvent(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
		//case WM_INPUT:
	case WM_MOUSEMOVE:
	{
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);
		_mousePos = vec2(xPos, yPos);
		break;
	}

	case WM_LBUTTONDOWN:
		_keyState[int(0)] = true;
		break;
	case WM_RBUTTONDOWN:
		_keyState[int(1)] = true;
		break;
	case WM_LBUTTONUP:
		_keyState[int(0)] = false;
		break;
	case WM_RBUTTONUP:
		_keyState[int(1)] = false;
		break;

	case WM_MOUSEWHEEL:
		break;
	}
}

void Input::_processKeyboardEvent(UINT message, WPARAM wParam, LPARAM lParam)
{
	uint8_t key(wParam);
	switch (message) {
		case WM_KEYUP:
			_keyState[key] = false;
			break;
		
		case WM_KEYDOWN:
			_keyState[key] = true;
			break;

		case WM_SYSKEYDOWN:
			_keyState[key] = true;
			break;
		
		case WM_SYSKEYUP:
			_keyState[key] = false;
			break;
	}
}
