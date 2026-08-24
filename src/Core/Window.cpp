#include "Window.h"

Window::Window(HINSTANCE instance, int width, int height)
	: m_instance(instance), m_width(width), m_height(height)
{}

bool Window::Create(const wchar_t* title, const wchar_t* className, int showCommand)
{
    WNDCLASS windowClass{};
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = m_instance;
    windowClass.lpszClassName = className;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);

    if (!RegisterClassW(&windowClass))
    {
        const DWORD error = GetLastError();

        if (error != ERROR_CLASS_ALREADY_EXISTS)
        {
            MessageBoxW(
                nullptr,
                L"Failed to register the window class.",
                title,
                MB_OK | MB_ICONERROR
            );

            return false;
        }
    }

    m_handle = CreateWindowEx(
        0,
        className,
        title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        m_width,
        m_height,
        nullptr,
        nullptr,
        m_instance,
        nullptr);

    if (!m_handle)
    {
        const DWORD error = GetLastError();

        wchar_t message[256]{};
        swprintf_s(
            message,
            L"Failed to create the application window.\nWin32 error: %lu",
            error
        );

        MessageBoxW(
            nullptr,
            message,
            title,
            MB_OK | MB_ICONERROR
        );

        return false;
    }

    ShowWindow(m_handle, showCommand);

    return true;
}

HWND Window::GetHandle() const noexcept
{
    return m_handle;
}

bool Window::ProcessMessages()
{
    MSG message{};

    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
    {
        if (message.message == WM_QUIT)
        {
            return false;
        }

        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return true;
}

LRESULT Window::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}
