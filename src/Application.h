#pragma once

#include "Core/Window.h"

class Application
{
public:
	// Window needs the module handle when it registers its class.
	Application(HINSTANCE instance);

	// Set up the window before the renderer starts using it.
	bool Initialize(int showCommand);

	// Keep processing messages and rendering until the window closes.
	int Run();

private:
	static const int windowWidth = 1280;
	static const int windowHeight = 720;

	Window m_window;
};