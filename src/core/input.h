#pragma once

#include "common.h"
#include "math.h"


enum class MouseButton
{
	Left,
	Right,
	Middle,

	Count,
};

struct InputState
{
	vec2 mouse;
	bool mouse_down[(int)MouseButton::Count];
	float mouse_scroll = 0.f;

	void set_mouse_position(i32 mouse_x, i32 mouse_y);
	void set_mouse_down(MouseButton button, bool down);
	void set_scroll(float scroll);
};

struct Input
{
	vec2 mouse_position() const { return current.mouse; }
	vec2 mouse_delta() const { return current.mouse - previous.mouse; }

	bool is_mouse_down(MouseButton button = MouseButton::Left) const { return current.mouse_down[(int)button]; }
	bool is_mouse_clicked(MouseButton button = MouseButton::Left) const { return current.mouse_down[(int)button] && !previous.mouse_down[(int)button]; }
	bool is_mouse_released(MouseButton button = MouseButton::Left) const { return !current.mouse_down[(int)button] && previous.mouse_down[(int)button]; }

	float mouse_scroll() const { return current.mouse_scroll; }

private:
	void begin_frame();

	InputState previous = {};
	InputState current = {};

	friend struct Window;
};
