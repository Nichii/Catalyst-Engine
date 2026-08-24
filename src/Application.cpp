#include "Application.h"
#include "Renderer.h"

Application::Application(HINSTANCE instance)
	: m_window(instance, windowWidth, windowHeight)
{}

bool Application::Initialize(int showCommand)
{
	return m_window.Create(L"Catalyst Engine", L"CatalystEngineWindow", showCommand);
}

int Application::Run()
{
	Renderer renderer;
	renderer.Initialize(m_window.GetHandle());

	while (m_window.ProcessMessages())
	{
		renderer.BeginFrame();
		renderer.Render();
		renderer.EndFrame();
	}

	return 0;
}
