#pragma once

#include "Core/Window.h"

class Application
{
public:
	Application(HINSTANCE instance);

	bool Initialize(int showCommand);
	int Run();

private:
	static const int windowWidth = 1280;
	static const int windowHeight = 720;

	Window m_window;
};