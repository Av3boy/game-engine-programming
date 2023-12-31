
#include "../include/IApplication.h"

IApplication* IApplication::m_pApp = nullptr;

IApplication::IApplication() :
	window(nullptr),
	m_windowWidth(0),
	m_windowHeight(0),
	m_bActive(false)
{
	m_pApp = this;
}

IApplication::~IApplication()
{
	m_pApp = nullptr;
}

void IApplication::Debug(const wchar_t* msg) { ::OutputDebugStringW(msg); }

void IApplication::Debug(const std::string& msg) { ::OutputDebugStringA(msg.c_str()); }
void IApplication::Debug(const char* msg) { ::OutputDebugStringA(msg); }

bool IApplication::Create(int32_t resX, int32_t resY, const std::string& title)
{
	window = MakeWindow(resX, resY, title);
	if (window)
	{
		m_windowWidth = resX;
		m_windowHeight = resY;

		if (OnCreate())
		{
			SetActive(true);
			return true;
		}
	}

	return false;
}

void IApplication::SetActive(bool isActive)
{
	m_bActive = isActive;

	m_Timer.BeginTimer();
}

void IApplication::Run()
{
	MSG msg;
	BOOL messageRetrieved = false;

	::PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE);

	while (msg.message != WM_QUIT)
	{
		if (IsActive())
			messageRetrieved = ::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);
		else
			messageRetrieved = ::GetMessage(&msg, nullptr, 0, 0);

		if (messageRetrieved)
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}

		if (msg.message != WM_QUIT)
		{
			m_Timer.EndTimer();
			m_Timer.BeginTimer();

			OnUpdate(m_Timer.GetElapsedSeconds());
			OnDraw();
		}

	}

	OnDestroy();
}

void IApplication::Close()
{
	::PostQuitMessage(0);
}

void IApplication::GetRandomSeed()
{
	srand((uint32_t)::GetTickCount64());
}

bool IApplication::IsKeyDown(uint32_t keyCode)
{
	return ::GetAsyncKeyState(keyCode);
}

bool IApplication::OnEvent(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
			SetActive(false);
		else if (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED)
		{
			RECT rect;
			::GetClientRect(GetWindow(), &rect);

			const int32_t windowHeight = rect.right - rect.left;
			const int32_t windowWidth  = rect.bottom - rect.top;

			if (windowWidth != m_windowWidth || windowHeight != m_windowHeight)
			{
				m_windowWidth = windowWidth;
				m_windowHeight = windowHeight;
			}

			SetActive(true);
		}

		break;

	case WM_KEYDOWN:
		OnKeyDown((uint32_t)wParam);
		break;
	}

	return false;
}

long IApplication::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	break;

	case WM_CREATE:
		::SetForegroundWindow(hwnd);
		break;

	case WM_CLOSE:
		::DestroyWindow(hwnd);
		return 0;
		break;

	default:
		break;
	}

	bool callDefWndProc = true;
	auto app = IApplication::GetApp();
	if (app)
	{
		callDefWndProc = !app->OnEvent(message, wParam, lParam);
	}

	if (callDefWndProc)
		return (long)::DefWindowProc(hwnd, message, wParam, lParam);

	return 0;
}

HWND IApplication::MakeWindow(int32_t width, int32_t height, const std::string& title)
{
	HINSTANCE hInst = ::GetModuleHandle(nullptr);
	DWORD windowStyle = 0;
	windowStyle =
		WS_OVERLAPPED |
		WS_CAPTION |
		WS_SYSMENU |
		WS_THICKFRAME |
		WS_MINIMIZEBOX |
		WS_MAXIMIZEBOX;

	WNDCLASS wc;
	memset(&wc, 0, sizeof(WNDCLASS));

	wc.style = CS_HREDRAW | CS_VREDRAW; // Refresh window horizontally and vertically.
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.hInstance = hInst;
	wc.hIcon = ::LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)::GetStockObject(BLACK_BRUSH);
	wc.lpszClassName = L"COREENGINE_WNDCLASS";

	if (!::RegisterClass(&wc))
	{
		return nullptr;
	}

	auto windowTitle = std::wstring(title.begin(), title.end());
	HWND window = ::CreateWindow(
		wc.lpszClassName,
		windowTitle.c_str(),
		windowStyle,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width,
		height,
		nullptr,
		nullptr,
		hInst,
		nullptr);

	if (!window) 
	{
		Debug("Failed to create window, exiting...");
		return nullptr;
	}

	// Make the window use the drawable area size as the provided sizes.
	::SetWindowLong(window, GWL_STYLE, windowStyle);
	RECT clientArea = { 0, 0, width, height };
	::AdjustWindowRectEx(&clientArea, windowStyle, FALSE, 0);
	::SetWindowPos(window, nullptr, 0, 0, clientArea.right - clientArea.left, clientArea.bottom - clientArea.top, SWP_NOZORDER | SWP_NOMOVE | SWP_SHOWWINDOW);

	::UpdateWindow(window);
	::ShowWindow(window, SW_SHOWNORMAL);

	return window;
}
