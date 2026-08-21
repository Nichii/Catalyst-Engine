#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include "Renderer.h"

using Microsoft::WRL::ComPtr;

LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    LPSTR,
    int nCmdShow)
{
    const wchar_t* className = L"CatalystWindow";

    WNDCLASS wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = className;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        className,
        L"Catalyst Engine",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1280,
        720,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    ShowWindow(hwnd, nCmdShow);

    Renderer renderer;
    renderer.Initialize(hwnd);

    MSG msg{};

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}