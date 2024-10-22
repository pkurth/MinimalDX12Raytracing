#pragma once

#include "common.h"
#include "input.h"

#include <functional>

struct WindowUpdate
{
	bool open = false;
	float delta_time_seconds = 0.f;
	u64 total_time_milliseconds = 0;

	operator bool() const { return open; }
};

struct Window
{
	Window(const TCHAR* name, u32 client_width, u32 client_height);
	Window(const Window&) = delete;
	Window(Window&& o) noexcept;
	virtual ~Window();

	void operator=(const Window&) = delete;
	void operator=(Window&& o) noexcept;

	bool initialized() const { return window_handle != 0; }
	WindowUpdate begin_frame(Input& input);

	void set_fullscreen(bool fullscreen);

	virtual void on_resize() {}



	HWND window_handle = 0;

	u32 client_width;
	u32 client_height;

	bool fullscreen = false;


private:


	std::chrono::time_point<std::chrono::high_resolution_clock> last_timepoint = std::chrono::high_resolution_clock::now();
	std::chrono::time_point<std::chrono::high_resolution_clock> start_timepoint = last_timepoint;

	WINDOWPLACEMENT window_placement;

	using MouseClickCallback = std::function<void(MouseButton mouse_button, bool down, i32 mouse_x, i32 mouse_y)>;
	using MouseMoveCallback = std::function<void(i32 mouse_x, i32 mouse_y)>;
	using MouseScrollCallback = std::function<void(float scroll)>;

	MouseClickCallback mouse_click_callback;
	MouseMoveCallback mouse_move_callback;
	MouseScrollCallback mouse_scroll_callback;

	friend static LRESULT CALLBACK window_callback(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);
};
