#include "Application.h"

#include <windows.h>

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    LPSTR,
    int nCmdShow)
{
    Application app(hInstance);

    if (!app.Initialize(nCmdShow))
        return 1;

    return app.Run();
}