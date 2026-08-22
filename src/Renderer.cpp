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

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
	rtvHeapDesc.NumDescriptors = bufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	hr = m_device->CreateDescriptorHeap(
		&rtvHeapDesc,
		IID_PPV_ARGS(&m_rtvHeap));

	if (FAILED(hr))
		throw std::runtime_error("Failed to create RTV descriptor heap!");

	m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

	for (UINT i = 0; i < bufferCount; i++)
	{
		hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]));

		if (FAILED(hr))
			throw std::runtime_error("Failed to get swap chain buffer!");

		m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
		rtvHandle.ptr += m_rtvDescriptorSize;
	}

	// Create fence and fence event
	hr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));

	if (FAILED(hr))
		throw std::runtime_error("Failed to create fence!");

	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	if (!m_fenceEvent)
		throw std::runtime_error("Failed to create fence event!");

	// Create command allocator and command list
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

void Renderer::WaitForPreviousFrame()
{
	const UINT64 fenceToWaitFor = ++m_fenceValue;

	HRESULT hr = m_commandQueue->Signal(m_fence.Get(), fenceToWaitFor);

	if (FAILED(hr))
		throw std::runtime_error("Failed to signal fence!");

	if (m_fence->GetCompletedValue() < fenceToWaitFor)
	{
		hr = m_fence->SetEventOnCompletion(fenceToWaitFor, m_fenceEvent);

		if (FAILED(hr))
			throw std::runtime_error("Failed to set fence event!");

		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
}

void Renderer::BeginFrame()
{
	// Reset command allocator and command list for the current frame
	m_commandAllocator->Reset();
	m_commandList->Reset(m_commandAllocator.Get(), nullptr);

	// Transition the render target to the render target state
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_commandList->ResourceBarrier(1, &barrier);

	// Get the render target view handle for the current frame
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += static_cast<SIZE_T>(m_frameIndex * m_rtvDescriptorSize);

	// Set the render target for the current frame
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// Clear the render target
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f }; // Temporary clear color
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
}

void Renderer::EndFrame()
{
	m_commandList->Close();
	ID3D12CommandList* commandLists[] = { m_commandList.Get() };

	m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	HRESULT hr = m_swapChain->Present(1, 0);

	if (FAILED(hr))
		throw std::runtime_error("Failed to present swap chain!");
	
	WaitForPreviousFrame();

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_commandList->ResourceBarrier(1, &barrier);
}
