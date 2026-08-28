#include "Application.h"

#include <windows.h>

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    LPSTR,
    int nCmdShow)
{
    // This is the GUI entry point used instead of a console main.
    Application app(hInstance);

    // There is nothing to run if the window could not be created.
    if (!app.Initialize(nCmdShow))
        return 1;

    // Run the application until the window is closed.
    return app.Run();
}