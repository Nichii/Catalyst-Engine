#pragma once

#include <windows.h>
#include <string_view>

class Window
{
public:
	Window(HINSTANCE instance, int width, int height);

	bool Create(const wchar_t* title, const wchar_t* className, int showCommand);
	HWND GetHandle() const noexcept;
	bool ProcessMessages();

private:
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	HINSTANCE m_instance = nullptr;
	HWND m_handle = nullptr;
	int m_width = 0;
	int m_height = 0;
};