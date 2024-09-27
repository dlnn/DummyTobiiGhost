#include <d3d11.h>
#include <windows.h>
#include <windowsx.h>
#include <iostream>
#include <dwmapi.h>

#include "TobiiRender.hpp"


static auto  _swapChainOccluded = false;
static UINT  _resizeWidth       = 0,    _resizeHeight = 0;
static UINT  _width             = 1200, _height       = 600;
static auto  _frameRate         = 120;
static auto  _frameDuration     = 1.0 / _frameRate;
static POINT _mousePos          = {0, 0};

static POINT _lastMousePos = {0, 0};

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_SIZE:
			if (wParam == SIZE_MINIMIZED)
				return 0;
			_resizeWidth  = static_cast<UINT>(LOWORD(lParam)); // Queue resize
			_resizeHeight = static_cast<UINT>(HIWORD(lParam));
			return 0;
		case WM_SYSCOMMAND:
			if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
				return 0;
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_NCHITTEST:
		{
			// 提供无边框窗口的拖动和调整大小的功能
			auto hit = DefWindowProc(hWnd, msg, wParam, lParam);
			if (hit == HTCLIENT)
			{
				POINT ptMouse = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
				RECT  rcWindow;
				GetWindowRect(hWnd, &rcWindow);

				// 窗口左上角坐标
				int x = ptMouse.x - rcWindow.left;
				int y = ptMouse.y - rcWindow.top;

				// 设置调整窗口大小的边缘宽度和高度
				constexpr auto border_width = 10;

				// 判断鼠标位置并返回适当的命中测试值
				if (x < border_width && y < border_width)
					return HTTOPLEFT;
				if (x > rcWindow.right - rcWindow.left - border_width && y < border_width)
					return HTTOPRIGHT;
				if (x < border_width && y > rcWindow.bottom - rcWindow.top - border_width)
					return HTBOTTOMLEFT;
				if (x > rcWindow.right - rcWindow.left - border_width && y > rcWindow.bottom - rcWindow.top - border_width)
					return HTBOTTOMRIGHT;
				if (y < border_width)
					return HTTOP;
				if (y > rcWindow.bottom - rcWindow.top - border_width)
					return HTBOTTOM;
				if (x < border_width)
					return HTLEFT;
				if (x > rcWindow.right - rcWindow.left - border_width)
					return HTRIGHT;
				return HTCAPTION;
			}
			return hit;
		}
	}
	return DefWindowProcW(hWnd, msg, wParam, lParam);
}

void Blur(HWND hwnd)
{
	struct ACCENTPOLICY
	{
		int na;
		int nf;
		int nc;
		int nA;
	};
	struct WINCOMPATTRDATA
	{
		int   na;
		PVOID pd;
		ULONG ul;
	};

	const auto hm = LoadLibrary(L"user32.dll");
	if (hm)
	{
		typedef BOOL (WINAPI*pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);

		const auto SetWindowCompositionAttribute = (pSetWindowCompositionAttribute)GetProcAddress(hm, "SetWindowCompositionAttribute");
		if (SetWindowCompositionAttribute)
		{
			ACCENTPOLICY policy = {3, 0, 0, 0};

			WINCOMPATTRDATA data = {19, &policy, sizeof(ACCENTPOLICY)};
			SetWindowCompositionAttribute(hwnd, &data);
		}
		FreeLibrary(hm);
	}
}


void UpdateMousePositionInWindow(HWND hwnd)
{
	POINT point;
	if (GetCursorPos(&point))
	{
		// 将屏幕坐标转换为窗口坐标
		if (ScreenToClient(hwnd, &point))
		{
			_mousePos = point;
		}
	}
}


int main()
{
	SetConsoleOutputCP(CP_UTF8);

	constexpr wchar_t CLASS_NAME[] = L"Sample_Window_Class";

	WNDCLASSW wc     = {};
	wc.lpfnWndProc   = WndProc;
	wc.hInstance     = GetModuleHandle(nullptr);
	wc.lpszClassName = CLASS_NAME;


	RegisterClassW(&wc);

	// @formatter:off
	auto hwnd = CreateWindowExW(
		WS_EX_LAYERED,
		CLASS_NAME,
		L"",
		WS_POPUP | WS_MAXIMIZEBOX | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		_width, _height,
		nullptr,
		nullptr,
		GetModuleHandle(nullptr),
		nullptr);
	// @formatter:on

	if (hwnd == nullptr)
	{
		std::wcerr << L"CreateWindowExW Failed\n";
		return 0;
	}


	SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);

	MARGINS margins = {-1};
	DwmExtendFrameIntoClientArea(hwnd, &margins);

	Blur(hwnd);

	TobiiRender tobiiRender(hwnd, _width, _height);

	if (!tobiiRender.Init())
	{
		std::wcerr << L"初始化 TobiiRender 失败\n";
	}

	TobiiRenderSettings settings = {};
	//settings.Enable = false;
	//settings.ShapeType = Heatmap;
	settings.Size = 0.8f;
	settings.Color = { 0.0f, 0.0f, 0.0f, 0.8f };
	settings.BackgroundColor = { 1.0f, 1.0f, 1.0f, 0.3f };

	tobiiRender.UpdateSettings(settings);

	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);

	LARGE_INTEGER lastTime;
	QueryPerformanceCounter(&lastTime);

	auto done = false;
	while (!done)
	{
		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				done = true;
		}
		if (done)
			break;


		if (_swapChainOccluded && tobiiRender.Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
		{
			Sleep(10);
			continue;
		}
		_swapChainOccluded = false;

		if (_resizeWidth != 0 && _resizeHeight != 0)
		{
			_width = _resizeWidth;
			_height = _resizeHeight;

			_resizeWidth = _resizeHeight = 0;

			tobiiRender.Resize(_width, _height);
		}


		LARGE_INTEGER currentTime;
		QueryPerformanceCounter(&currentTime);

		// 计算经过的时间
		auto elapsedTime = static_cast<double>(currentTime.QuadPart - lastTime.QuadPart) / frequency.QuadPart;

		if (elapsedTime >= _frameDuration)
		{
			lastTime = currentTime;

			UpdateMousePositionInWindow(hwnd);


			{
				auto isChanged = _mousePos.x != _lastMousePos.x || _mousePos.y != _lastMousePos.y;


#pragma warning(disable: 4018)
				auto inRect = _mousePos.x >= 0 && _mousePos.x <= _width && _mousePos.y >= 0 && _mousePos.y <= _height;
#pragma warning(default :4018)

				//std::cout << "Mouse position in window: (" << _mousePos.x << ", " << _mousePos.y << ")" << std::endl;

				auto isActive = inRect && isChanged;

				const Point point = {_mousePos.x * 1.0f, _mousePos.y * 1.0f};


				tobiiRender.PushGazePoint(isActive, point);

				if (isChanged)
				{
					_lastMousePos = _mousePos;
				}


				tobiiRender.Render();
			}


			_swapChainOccluded = tobiiRender.Present(0,0) == DXGI_STATUS_OCCLUDED;
		}
	}
}
