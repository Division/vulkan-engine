#pragma once

#include "glm/glm.hpp"
#include <memory>
#include <windows.h>

using namespace glm;

enum class Key : int {
  Down = 0,
  Up,
  Left,
  Right,
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

  MouseLeft,
  MouseRight,
};

class Window;

class Input {
public:
  friend class Window;

  int keyDown(Key key) const;
  const vec2 mousePosition() const { return _mousePos; }
  const vec2 mouseDelta() const { return _mouseDelta; }
private:
	void _update();
	void _processMouseEvent(UINT message, WPARAM wParam, LPARAM lParam);
	void _processKeyboardEvent(UINT message, WPARAM wParam, LPARAM lParam);

private:
  std::shared_ptr<Window>_window;

  bool _keyState[256];

  vec2 _mousePos;
  vec2 _prevMousePos;
  vec2 _mouseDelta;
};

