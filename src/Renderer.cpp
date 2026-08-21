#include "Renderer.h"

#include <stdexcept>

void Renderer::Initialize(HWND hwnd)
{
	UINT flags = 0;

#ifdef _DEBUG
	flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	// Create DXGI Factory
	HRESULT hr = CreateDXGIFactory2(flags, IID_PPV_ARGS(&m_factory));

	if (FAILED(hr))
		throw std::runtime_error("Failed to create DXGI Factory!");

	hr = D3D12CreateDevice(
		nullptr,
		D3D_FEATURE_LEVEL_12_0,
		IID_PPV_ARGS(&m_device)
	);

	if (FAILED(hr))
		throw std::runtime_error("Failed to create D3D12 Device!");

	// Create command queue
	D3D12_COMMAND_QUEUE_DESC queueDesc{};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	hr = m_device->CreateCommandQueue(
		&queueDesc,
		IID_PPV_ARGS(&m_commandQueue)
	);

	if (FAILED(hr))
		throw std::runtime_error("Failed to create command queue!");

	// Create swap chain
	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swapChain;

	static constexpr UINT bufferCount = 2;

	Microsoft::WRL::ComPtr<ID3D12Resource> renderTargets[bufferCount];

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.BufferCount = bufferCount;
	swapChainDesc.Width = 1280;
	swapChainDesc.Height = 720;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;

	hr = m_factory->CreateSwapChainForHwnd(
		m_commandQueue.Get(),
		hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	);

	if (FAILED(hr))
		throw std::runtime_error("Failed to create swap chain!");

	swapChain.As(&m_swapChain);

	// Create command allocator and command list
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;

	hr = m_device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&m_commandAllocator)
	);

	if (FAILED(hr))
		throw std::runtime_error("Failed to create command allocator!");

	hr = m_device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_commandAllocator.Get(),
		nullptr,
		IID_PPV_ARGS(&m_commandList)
	);

	if (FAILED(hr))
		throw std::runtime_error("Failed to create graphics command list!");

	m_commandList->Close();
}