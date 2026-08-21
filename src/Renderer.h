#pragma once

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

class Renderer
{
public:
	void Initialize(HWND hwnd);

private:
	Microsoft::WRL::ComPtr<IDXGIFactory7> m_factory;
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
};