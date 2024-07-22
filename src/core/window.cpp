#include "window.h"



static LRESULT CALLBACK window_callback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

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

bool Window::begin_frame()
{
	MSG msg = { 0 };
	while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			return false;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return initialized();
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



static LRESULT CALLBACK window_callback(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	Window* window = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

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
				window->client_width = LOWORD(lParam);
				window->client_height = HIWORD(lParam);
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
		default:
		{
			result = DefWindowProc(hwnd, msg, wParam, lParam);
		} break;
	}

	return result;
}


