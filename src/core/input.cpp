#include "input.h"

void Input::begin_frame()
{
	memcpy(&previous, &current, sizeof(InputState));

	current.mouse_scroll = 0.f;
}

void InputState::set_mouse_position(i32 mouse_x, i32 mouse_y)
{
	this->mouse.x = (float)mouse_x;
	this->mouse.y = (float)mouse_y;
}

void InputState::set_mouse_down(MouseButton button, bool down)
{
	this->mouse_down[(int)button] = down;
}

void InputState::set_scroll(float scroll)
{
	this->mouse_scroll = scroll;
}
