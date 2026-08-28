#include "Window.h"

Window::Window(HINSTANCE instance, int width, int height)
	: m_instance(instance), m_width(width), m_height(height)
{}

bool Window::Create(const wchar_t* title, const wchar_t* className, int showCommand)
{
    // This class description supplies the callback and instance used by the window.
    WNDCLASS windowClass{};
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = m_instance;
    windowClass.lpszClassName = className;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);

    if (!RegisterClassW(&windowClass))
    {
        const DWORD error = GetLastError();

        // Reusing an already-registered class is fine; other registration errors are not.
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

    // HWND is the handle the rest of the engine uses to refer to this window.
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

    // Let the caller choose the initial show state.
    ShowWindow(m_handle, showCommand);

    return true;
}

HWND Window::GetHandle() const noexcept
{
    return m_handle;
}

void Window::GetClientSize(int& width, int& height) const noexcept
{
    // The swap chain should match the client area, not the title bar and borders.
    RECT clientRect{};
    if (m_handle && GetClientRect(m_handle, &clientRect))
    {
        width = clientRect.right - clientRect.left;
        height = clientRect.bottom - clientRect.top;
    }
    else
    {
        width = m_width;
        height = m_height;
    }
}

bool Window::ProcessMessages()
{
    MSG message{};

    // PeekMessage lets us render immediately when there are no messages waiting.
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
    {
        if (message.message == WM_QUIT)
        {
            // WindowProc posts WM_QUIT when the user closes the window.
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
        // Turn the close notification into the message that ends the main loop.
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}
