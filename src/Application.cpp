#include "Application.h"
#include "Renderer/Renderer.h"

Application::Application(HINSTANCE instance)
	: m_window(instance, windowWidth, windowHeight)
{}

bool Application::Initialize(int showCommand)
{
	// Keep window setup separate so a Win32 failure can be reported before creating GPU resources.
	return m_window.Create(L"Catalyst Engine", L"CatalystEngineWindow", showCommand);
}

int Application::Run()
{
	Renderer renderer;
	renderer.Initialize(m_window.GetHandle());

	// ProcessMessages does not wait, so the renderer can keep producing frames when the queue is empty.
	while (m_window.ProcessMessages())
	{
		// A frame is prepared, recorded, and submitted in that order.
		renderer.BeginFrame();
		renderer.Render();
		renderer.EndFrame();
	}

	return 0;
}
