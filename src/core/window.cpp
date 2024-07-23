#include "window.h"

#include <Windowsx.h>


static LRESULT CALLBACK window_callback(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);

Window::Window(const TCHAR* name, u32 client_width, u32 client_height)
{
	static const TCHAR* window_class_name = TEXT("APP WINDOW");

	static bool window_class_initialized = []()
	{
		WNDCLASSEX window_class;
		ZeroMemory(&window_class, sizeof(WNDCLASSEX));
		window_class.cbSize = sizeof(WNDCLASSEX);
		window_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
		window_class.lpfnWndProc = window_callback;
		window_class.hInstance = GetModuleHandle(NULL);
		window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
		window_class.lpszClassName = window_class_name;

		if (!RegisterClassEx(&window_class))
		{
			std::cerr << "Failed to create window class.\n";
			return false;
		}

		return true;
	}();

	if (!window_class_initialized)
	{
		return;
	}


	this->client_width = client_width;
	this->client_height = client_height;

	DWORD window_style = WS_OVERLAPPEDWINDOW;

	RECT r = { 0, 0, (LONG)client_width, (LONG)client_height };
	AdjustWindowRect(&r, window_style, FALSE);
	int width = r.right - r.left;
	int height = r.bottom - r.top;

	window_handle = CreateWindowEx(0, window_class_name, name, window_style,
#if 1
		CW_USEDEFAULT, CW_USEDEFAULT,
#else
		0, 0
#endif
		width, height,
		0, 0, 0, 0);

	if (!window_handle)
	{
		std::cerr << "Failed to create window.\n";
		return;
	}

	SetWindowLongPtr(window_handle, GWLP_USERDATA, (LONG_PTR)this);
	ShowWindow(window_handle, SW_SHOW);
}

Window::Window(Window&& o) noexcept
{
	*this = std::move(o);
}

Window::~Window()
{
	if (initialized())
	{
		DestroyWindow(window_handle);
	}
}

void Window::operator=(Window&& o) noexcept
{
	if (window_handle)
	{
		DestroyWindow(window_handle);
	}

	window_handle = std::exchange(o.window_handle, HWND(0));
	client_width = o.client_width;
	client_height = o.client_height;
	fullscreen = o.fullscreen;
	window_placement = o.window_placement;

	if (initialized())
	{
		SetWindowLongPtr(window_handle, GWLP_USERDATA, (LONG_PTR)this);
	}
}

WindowUpdate Window::begin_frame(Input& input)
{
	mouse_click_callback = [&](MouseButton mouse_button, bool down, i32 mouse_x, i32 mouse_y)
	{
		input.current.set_mouse_down(mouse_button, down);
	};
	mouse_move_callback = [&](i32 mouse_x, i32 mouse_y)
	{
		input.current.set_mouse_position(mouse_x, mouse_y);
	};
	mouse_scroll_callback = [&](float scroll)
	{
		input.current.set_scroll(scroll);
	};

	input.begin_frame();

	MSG msg = { 0 };
	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			return { };
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	mouse_click_callback = {};
	mouse_move_callback = {};
	mouse_scroll_callback = {};


	std::chrono::time_point<std::chrono::high_resolution_clock> now = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> delta = now - last_timepoint;
	float delta_time = (float)delta.count();
	last_timepoint = now;

	u64 total_runtime = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_timepoint).count();

	return { initialized(), delta_time, total_runtime };
}

void Window::set_fullscreen(bool fullscreen)
{
	if (!initialized() || fullscreen == this->fullscreen)
	{
		return;
	}

	this->fullscreen = fullscreen;

	DWORD style = GetWindowLong(window_handle, GWL_STYLE);

	if (fullscreen)
	{
		if (style & WS_OVERLAPPEDWINDOW)
		{
			MONITORINFO monitorInfo = { sizeof(MONITORINFO) };
			if (GetWindowPlacement(window_handle, &window_placement) &&
				GetMonitorInfo(MonitorFromWindow(window_handle, MONITOR_DEFAULTTOPRIMARY), &monitorInfo))
			{
				SetWindowLong(window_handle, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
				SetWindowPos(window_handle, HWND_TOP,
					monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
					monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
					monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
					SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
			}
		}
	}
	else
	{
		if (!(style & WS_OVERLAPPEDWINDOW))
		{
			SetWindowLong(window_handle, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
			SetWindowPlacement(window_handle, &window_placement);
			SetWindowPos(window_handle, 0, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
				SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		}
	}

}



static LRESULT CALLBACK window_callback(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
	LRESULT result = 0;

	Window* window = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	auto handle_mouse_click = [window](MouseButton button, bool down, LPARAM l_param)
	{
		if (window && window->initialized() && window->mouse_click_callback)
		{
			i32 x = GET_X_LPARAM(l_param);
			i32 y = GET_Y_LPARAM(l_param);
			window->mouse_click_callback(button, down, x, y);

			if (down)
			{
				SetCapture(window->window_handle);
			}
			else
			{
				ReleaseCapture();
			}
		}
	};

	auto handle_mouse_move = [window](LPARAM l_param)
	{
		if (window && window->initialized() && window->mouse_move_callback)
		{
			i32 x = GET_X_LPARAM(l_param);
			i32 y = GET_Y_LPARAM(l_param);
			window->mouse_move_callback(x, y);
		}
	};

	auto handle_mouse_scroll = [window](WPARAM w_param)
	{
		if (window && window->initialized() && window->mouse_scroll_callback)
		{
			float scroll = (float)GET_WHEEL_DELTA_WPARAM(w_param) / WHEEL_DELTA;
			window->mouse_scroll_callback(scroll);
		}
	};

	switch (msg)
	{
		// The default window procedure will play a system notification sound 
		// when pressing the Alt+Enter keyboard combination if this message is 
		// not handled.
		case WM_SYSCHAR:
			break;
		case WM_SIZE:
		{
			if (window && window->initialized())
			{
				window->client_width = LOWORD(l_param);
				window->client_height = HIWORD(l_param);
				window->on_resize();
			}
		} break;
		case WM_CLOSE:
		{
			DestroyWindow(hwnd);
		} break;
		case WM_DESTROY:
		{
			if (window && window->initialized())
			{
				window->window_handle = 0;
			}
		} break;

		case WM_LBUTTONDOWN:
		{
			handle_mouse_click(MouseButton::Left, true, l_param);
		} break;
		case WM_LBUTTONUP:
		{
			handle_mouse_click(MouseButton::Left, false, l_param);
		} break;
		case WM_RBUTTONDOWN:
		{
			handle_mouse_click(MouseButton::Right, true, l_param);
		} break;
		case WM_RBUTTONUP:
		{
			handle_mouse_click(MouseButton::Right, false, l_param);
		} break;
		case WM_MBUTTONDOWN:
		{
			handle_mouse_click(MouseButton::Middle, true, l_param);
		} break;
		case WM_MBUTTONUP:
		{
			handle_mouse_click(MouseButton::Middle, false, l_param);
		} break;

		case WM_MOUSEMOVE:
		{
			handle_mouse_move(l_param);
		} break;

		case WM_MOUSEWHEEL:
		{
			handle_mouse_scroll(w_param);
		} break;

		default:
		{
			result = DefWindowProc(hwnd, msg, w_param, l_param);
		} break;
	}

	return result;
}


