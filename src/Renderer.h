#pragma once

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <cstring>
#include <d3dcompiler.h>

class Renderer
{
public:
	void Initialize(HWND hwnd);
	void WaitForPreviousFrame();
	void BeginFrame();
	void EndFrame();

private:
	static constexpr UINT bufferCount = 2;

	UINT m_rtvDescriptorSize = 0;
	UINT m_frameIndex = 0;
	UINT64 m_fenceValue = 0;
	HANDLE m_fenceEvent = nullptr;

	Microsoft::WRL::ComPtr<IDXGIFactory7> m_factory;
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[bufferCount];
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView{};

	Microsoft::WRL::ComPtr<ID3DBlob> m_vertexShader;
	Microsoft::WRL::ComPtr<ID3DBlob> m_pixelShader;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
};

struct Vertex
{
	float position[3];
	float color[3];
};