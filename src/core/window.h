#pragma once

#include "common.h"

struct Window
{
	Window(const TCHAR* name, u32 client_width, u32 client_height);
	Window(const Window&) = delete;
	Window(Window&& o) noexcept;
	virtual ~Window();

	void operator=(const Window&) = delete;
	void operator=(Window&& o) noexcept;

	bool initialized() const { return window_handle != 0; }
	bool begin_frame();

	void set_fullscreen(bool fullscreen);

	virtual void on_resize() {}



	HWND window_handle = 0;

	u32 client_width;
	u32 client_height;

	bool fullscreen = false;
	WINDOWPLACEMENT window_placement;
};
