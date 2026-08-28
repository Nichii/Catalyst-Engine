#pragma once

#include <windows.h>
#include <string_view>

class Window
{
public:
	// The requested size includes the window frame; the drawable client area is slightly smaller.
	Window(HINSTANCE instance, int width, int height);

	// Register the class, create the native window, and show it.
	bool Create(const wchar_t* title, const wchar_t* className, int showCommand);

	// Renderer passes this handle to DXGI and uses it for window-related queries.
	HWND GetHandle() const noexcept;

	// Return the part of the window that can actually be rendered into.
	void GetClientSize(int& width, int& height) const noexcept;

	// Drain pending Win32 messages without stopping the render loop.
	bool ProcessMessages();

private:
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	HINSTANCE m_instance = nullptr;
	HWND m_handle = nullptr;
	int m_width = 0;
	int m_height = 0;
};