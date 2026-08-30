#include "Renderer.h"
#include "../Core/Dx12Utils.h"

#include <stdexcept>
#include <d3dcompiler.h>
#include <algorithm>
#include <format>

Renderer::~Renderer()
{
	// COM resources release automatically, the Win32 event is the only non-COM handle owned here.
	if (m_fenceEvent)
	{
		CloseHandle(m_fenceEvent);
		m_fenceEvent = nullptr;
	}
}

void Renderer::Initialize(HWND hwnd)
{
	if (!hwnd)
		throw std::invalid_argument("Renderer requires a valid window handle");
	m_hwnd = hwnd;

	// Turn on the DXGI debug factory in Debug builds.
	UINT flags = 0;

#ifdef _DEBUG
	flags |= DXGI_CREATE_FACTORY_DEBUG;

	{
		Microsoft::WRL::ComPtr<ID3D12Debug> debugController;

		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
		}
	}
#endif
	
	// Build things in dependency order: device, swap chain, buffers, descriptors, then PSOs.
	CreateDevice(flags);
	CreateSwapChain(hwnd);
	CreateCommandObjects();
	CreateRenderTargets();

	CreateCameraBuffer();
	CreateObjectBuffer();
	CreateIndirectBuffers();

	CreateDescriptorHeap();
	CreateCameraCBV();
	CreateObjectSRV();
	CreateRootSignature();
	CreatePipelineState();

	CreateCullingRootSignature();
	CreateCullingPipeline();
	CreateCommandSignature();
}

void Renderer::WaitForPreviousFrame()
{
	// Do not reuse the command allocator while the GPU is still reading it.
	const UINT64 fenceToWaitFor = ++m_fenceValue;

	HRESULT hr = m_commandQueue->Signal(m_fence.Get(), fenceToWaitFor);

	ThrowIfFailed(hr, "Signal fence");

	if (m_fence->GetCompletedValue() < fenceToWaitFor)
	{
		hr = m_fence->SetEventOnCompletion(fenceToWaitFor, m_fenceEvent);

		ThrowIfFailed(hr, "Set fence event");

		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
}

void Renderer::BeginFrame()
{
	// Poll the client size so resizing works without a separate resize callback.
	RECT clientRect{};
	if (GetClientRect(m_hwnd, &clientRect))
	{
		const UINT width = static_cast<UINT>(std::max<LONG>(1, clientRect.right - clientRect.left));
		const UINT height = static_cast<UINT>(std::max<LONG>(1, clientRect.bottom - clientRect.top));
		if (width != m_width || height != m_height)
			Resize(width, height);
	}
	UpdateCamera();

	// The allocator and list are reused after the previous frame has finished.
	HRESULT hr = m_commandAllocator->Reset();
	ThrowIfFailed(hr, "Reset command allocator");

	hr = m_commandList->Reset(m_commandAllocator.Get(), nullptr);
	ThrowIfFailed(hr, "Reset command list");

	// A back buffer must leave PRESENT state before we render into it.
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_commandList->ResourceBarrier(1, &barrier);

	// Pick the RTV belonging to the current back buffer.
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += static_cast<SIZE_T>(m_frameIndex * m_rtvDescriptorSize);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

	// Send color and depth output to the current frame's views.
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	// Keep rasterization aligned with the current client size.
	D3D12_VIEWPORT viewport{};
	viewport.Width = static_cast<float>(m_width);
	viewport.Height = static_cast<float>(m_height);
	viewport.MaxDepth = 1.0f;
	m_commandList->RSSetViewports(1, &viewport);

	D3D12_RECT scissorRect{ 0L, 0L, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
	m_commandList->RSSetScissorRects(1, &scissorRect);

	// Start with a clean color and depth buffer.
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void Renderer::Render()
{
	// F1 switches between the indirect path and a simple CPU baseline.
	static bool previousToggleState = false;
	const bool toggleState = (GetAsyncKeyState(VK_F1) & 0x8000) != 0;
	if (toggleState && !previousToggleState)
		m_useIndirect = !m_useIndirect;
	previousToggleState = toggleState;
	if (m_useIndirect)
		// First build the command stream that ExecuteIndirect will consume.
		DispatchCulling();

	// Set up the graphics state shared by both submission paths.
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	ID3D12DescriptorHeap* descriptorHeaps[] = { m_srvHeap.Get() };

	m_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	// Root parameter 0: camera matrix.
	m_commandList->SetGraphicsRootConstantBufferView(0, m_cameraBuffer->GetGPUVirtualAddress());

	// Descriptor 1 contains the object transforms.
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_srvHeap->GetGPUDescriptorHandleForHeapStart();
	gpuHandle.ptr += m_srvDescriptorSize;

	// Root parameter 1: object-buffer SRV.
	m_commandList->SetGraphicsRootDescriptorTable(1, gpuHandle);

	m_commandList->SetPipelineState(m_pipelineState.Get());
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->IASetIndexBuffer(&m_indexBufferView);
	if (m_useIndirect)
	{
		// The GPU count decides how many of the generated commands are executed.
		m_commandList->ExecuteIndirect(m_commandSignature.Get(), objectCount, m_indirectArgsBuffer.Get(), 0,
			m_indirectCountBuffer.Get(), 0);
	}
	else
	{
		for (uint32_t objectIndex = 0; objectIndex < objectCount; ++objectIndex)
		{
			m_commandList->SetGraphicsRoot32BitConstant(2, objectIndex, 0);
			m_commandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
		}
	}

	++m_frameCounter;
	const auto now = std::chrono::steady_clock::now();
	if (now - m_lastDiagnostic >= std::chrono::seconds(1))
	{
		const auto elapsed = std::chrono::duration<float>(now - m_lastDiagnostic).count();
		const float fps = static_cast<float>(m_frameCounter) / elapsed;
		const std::string message = std::format("CatalystEngine: mode={}, objects={}, CPU draws={}, FPS={:.1f}\n",
			m_useIndirect ? "GPU indirect" : "CPU baseline", objectCount,
			m_useIndirect ? 1u : objectCount, fps);
		OutputDebugStringA(message.c_str());
		m_frameCounter = 0;
		m_lastDiagnostic = now;
	}
}

void Renderer::EndFrame()
{
	// Submit this frame, present it, and move to the next back buffer.
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_commandList->ResourceBarrier(1, &barrier);

	HRESULT hr = m_commandList->Close();
	ThrowIfFailed(hr, "Close command list");

	ID3D12CommandList* commandLists[] = { m_commandList.Get() };

	m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	hr = m_swapChain->Present(1, 0);

	ThrowIfFailed(hr, "Present swap chain");
	
	WaitForPreviousFrame();

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void Renderer::CreateDevice(UINT flags)
{
	// Create DXGI Factory
	HRESULT hr = CreateDXGIFactory2(flags, IID_PPV_ARGS(&m_factory));

	ThrowIfFailed(hr, "Create DXGI factory");

	hr = D3D12CreateDevice(
		nullptr,
		D3D_FEATURE_LEVEL_12_0,
		IID_PPV_ARGS(&m_device)
	);

	ThrowIfFailed(hr, "Create D3D12 device");
}

void Renderer::CreateSwapChain(HWND hwnd)
{
	// Create command queue
	D3D12_COMMAND_QUEUE_DESC queueDesc{};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	HRESULT hr = m_device->CreateCommandQueue(
		&queueDesc,
		IID_PPV_ARGS(&m_commandQueue)
	);

	ThrowIfFailed(hr, "Create command queue");

	// Create swap chain
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.BufferCount = bufferCount;
	swapChainDesc.Width = defaultWidth;
	swapChainDesc.Height = defaultHeight;
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

	ThrowIfFailed(hr, "Create swap chain");

	swapChain.As(&m_swapChain);
}

void Renderer::CreateCommandObjects()
{
	// Create command allocator and command list
	HRESULT hr = m_device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&m_commandAllocator)
	);

	ThrowIfFailed(hr, "Create command allocator");

	hr = m_device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_commandAllocator.Get(),
		nullptr,
		IID_PPV_ARGS(&m_commandList)
	);

	ThrowIfFailed(hr, "Create graphics command list");

	// Close the command list as it will be reset before recording commands
	m_commandList->Close();

	ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)), "Create fence");
	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (!m_fenceEvent)
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()), "Create fence event");
}

void Renderer::CreateRenderTargets()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
	rtvHeapDesc.NumDescriptors = bufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	HRESULT hr = m_device->CreateDescriptorHeap(
		&rtvHeapDesc,
		IID_PPV_ARGS(&m_rtvHeap));

	ThrowIfFailed(hr, "Create RTV descriptor heap");

	m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

	for (UINT i = 0; i < bufferCount; i++)
	{
		hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]));

		ThrowIfFailed(hr, "Failed to get swap chain buffer!");

		m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
		rtvHandle.ptr += m_rtvDescriptorSize;
	}
	CreateDepthStencil();
}

void Renderer::Resize(UINT width, UINT height)
{
	// ResizeBuffers requires all references to the old back buffers to be released first.
	WaitForPreviousFrame();
	for (auto& renderTarget : m_renderTargets)
		renderTarget.Reset();
	m_depthStencil.Reset();
	ThrowIfFailed(m_swapChain->ResizeBuffers(bufferCount, width, height,
		DXGI_FORMAT_R8G8B8A8_UNORM, 0), "Resize swap chain buffers");
	m_width = width;
	m_height = height;
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
	CreateRenderTargets();
}

void Renderer::CreateDepthStencil()
{
	// Depth is a separate resource from the swap chain because it is not presented.
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.NumDescriptors = 1;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_dsvHeap)), "Create DSV heap");

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = m_width;
	desc.Height = m_height;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_D32_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil.Depth = 1.0f;
	ThrowIfFailed(m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
		&desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
		IID_PPV_ARGS(&m_depthStencil)), "Create depth stencil");
	D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc{};
	viewDesc.Format = DXGI_FORMAT_D32_FLOAT;
	viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	m_device->CreateDepthStencilView(m_depthStencil.Get(), &viewDesc,
		m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void Renderer::CreateCameraBuffer()
{
	// Create camera matrix
	DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH
	(
		DirectX::XMVectorSet(0.0f, 0.0f, -120.0f, 1.0f),
		DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f),
		DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
	);
	DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovLH
	(
		DirectX::XMConvertToRadians(60.0f),
		static_cast<float>(defaultWidth) / static_cast<float>(defaultHeight),
		0.1f,
		1000.0f
	);
	DirectX::XMStoreFloat4x4
	(
		&m_cameraData.viewProjection,
		XMMatrixTranspose(view * projection)
	);

	// Create the constant buffer
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC bufferDesc{};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = 256; // Constant-buffer views must be aligned to 256 bytes
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	HRESULT hr = m_device->CreateCommittedResource
	(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_cameraBuffer)
	);

	ThrowIfFailed(hr, "Create camera buffer failed");

	// Upload the camera
	void* mappedData = nullptr;

	D3D12_RANGE readRange{};

	hr = m_cameraBuffer->Map(0, &readRange, &mappedData);

	ThrowIfFailed(hr, "Map camera buffer failed");

	memcpy(mappedData, &m_cameraData, sizeof(CameraData));

	m_cameraBuffer->Unmap(0, nullptr);
}

void Renderer::UpdateCamera()
{
	// Arrow keys orbit and zoom; WASD pans the point the camera is looking at.
	static float distance = 120.0f;
	static float angle = 0.0f;
	static float targetX = 0.0f;
	static float targetY = 0.0f;
	static float targetZ = 0.0f;
	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
		angle -= 0.02f;
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
		angle += 0.02f;
	if (GetAsyncKeyState(VK_UP) & 0x8000)
		distance = std::max(5.0f, distance - 0.25f);
	if (GetAsyncKeyState(VK_DOWN) & 0x8000)
		distance = std::min(500.0f, distance + 0.25f);
	const float moveSpeed = 0.75f;
	const float sinAngle = sinf(angle);
	const float cosAngle = cosf(angle);
	const float rightX = cosAngle;
	const float rightZ = sinAngle;
	const float forwardX = -sinAngle;
	const float forwardZ = cosAngle;
	if (GetAsyncKeyState('A') & 0x8000)
	{
		targetX -= rightX * moveSpeed;
		targetZ -= rightZ * moveSpeed;
	}
	if (GetAsyncKeyState('D') & 0x8000)
	{
		targetX += rightX * moveSpeed;
		targetZ += rightZ * moveSpeed;
	}
	if (GetAsyncKeyState('W') & 0x8000)
	{
		targetX += forwardX * moveSpeed;
		targetZ += forwardZ * moveSpeed;
	}
	if (GetAsyncKeyState('S') & 0x8000)
	{
		targetX -= forwardX * moveSpeed;
		targetZ -= forwardZ * moveSpeed;
	}

	const DirectX::XMVECTOR target = DirectX::XMVectorSet(targetX, targetY, targetZ, 1.0f);
	const DirectX::XMVECTOR position = DirectX::XMVectorSet(
		targetX + sinAngle * distance, targetY, targetZ - cosAngle * distance, 1.0f);
	const DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(
		position, target, DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	const DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovLH(
		DirectX::XMConvertToRadians(60.0f),
		static_cast<float>(m_width) / static_cast<float>(m_height), 0.1f, 1000.0f);
	DirectX::XMStoreFloat4x4(&m_cameraData.viewProjection,
		DirectX::XMMatrixTranspose(view * projection));

	void* mappedData = nullptr;
	D3D12_RANGE readRange{};
	ThrowIfFailed(m_cameraBuffer->Map(0, &readRange, &mappedData), "Map camera buffer");
	memcpy(mappedData, &m_cameraData, sizeof(CameraData));
	m_cameraBuffer->Unmap(0, nullptr);
}

void Renderer::CreateObjectBuffer()
{
	// Geometry and object transforms live in upload heaps in this small sample for simple CPU initialization.
	// Create heap properties and resource description for the vertex buffer
	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC bufferDesc{};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Alignment = 0;
	bufferDesc.Width = vertexBufferSize;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.SampleDesc.Quality = 0;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	HRESULT hr = m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_vertexBuffer)
	);

	ThrowIfFailed(hr, "Failed to create vertex buffer!");

	// Copy vertex data to the buffer
	void* mappedData = nullptr;

	D3D12_RANGE readRange{};

	hr = m_vertexBuffer->Map(0, &readRange, &mappedData);

	ThrowIfFailed(hr, "Failed to map vertex buffer!");

	memcpy(mappedData, vertices, vertexBufferSize);
	m_vertexBuffer->Unmap(0, nullptr);

	// Create vertex buffer view
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = vertexBufferSize;

	// Create index buffer
	bufferDesc.Width = indexBufferSize;

	hr = m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_indexBuffer)
	);

	ThrowIfFailed(hr, "Failed to create index buffer!");

	// Copy index data to the buffer
	hr = m_indexBuffer->Map(0, &readRange, &mappedData);

	ThrowIfFailed(hr, "Failed to map index buffer!");

	memcpy(mappedData, indices, indexBufferSize);
	m_indexBuffer->Unmap(0, nullptr);

	// Create index buffer view
	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.SizeInBytes = indexBufferSize;
	m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;

	// Compile shaders
	hr = D3DCompileFromFile(
		L"shaders/Triangle.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"VSMain",
		"vs_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS,
		0,
		&m_vertexShader,
		nullptr
	);

	ThrowIfFailed(hr, "Failed to compile vertex shader!");

	hr = D3DCompileFromFile(
		L"shaders/Triangle.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"PSMain",
		"ps_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS,
		0,
		&m_pixelShader,
		nullptr
	);

	ThrowIfFailed(hr, "Failed to compile pixel shader!");

	// Create object data buffer
	std::vector<ObjectData> objects;
	objects.resize(objectCount);

	for (uint32_t i = 0; i < objectCount; ++i)
	{
		ObjectData object{};

		constexpr uint32_t sceneWidth = 20;
		constexpr uint32_t sceneHeight = 20;
		constexpr uint32_t sceneDepth = 25;
		constexpr float objectSpacing = 2.5f;
		const uint32_t xIndex = i % sceneWidth;
		const uint32_t yIndex = (i / sceneWidth) % sceneHeight;
		const uint32_t zIndex = (i / (sceneWidth * sceneHeight)) % sceneDepth;
		const float x = (static_cast<float>(xIndex) - (sceneWidth - 1) * 0.5f) * objectSpacing;
		const float y = (static_cast<float>(yIndex) - (sceneHeight - 1) * 0.5f) * objectSpacing;
		const float z = (static_cast<float>(zIndex) - (sceneDepth - 1) * 0.5f) * objectSpacing;

		// World transform
		DirectX::XMMATRIX world = DirectX::XMMatrixTranslation(x, y, z);

		DirectX::XMStoreFloat4x4(&object.worldMatrix, DirectX::XMMatrixTranspose(world));

		// Bounding sphere
		object.bounds = { x, y, z, 0.866f };

		objects[i] = object;
	}

	D3D12_HEAP_PROPERTIES objectHeap{};
	objectHeap.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC objectBufferDesc{};
	objectBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	objectBufferDesc.Alignment = 0;
	objectBufferDesc.Width = objectCount * sizeof(ObjectData);
	objectBufferDesc.Height = 1;
	objectBufferDesc.DepthOrArraySize = 1;
	objectBufferDesc.MipLevels = 1;
	objectBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	objectBufferDesc.SampleDesc.Count = 1;
	objectBufferDesc.SampleDesc.Quality = 0;
	objectBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	objectBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	hr = m_device->CreateCommittedResource(
		&objectHeap,
		D3D12_HEAP_FLAG_NONE,
		&objectBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_objectDataBuffer)
	);

	ThrowIfFailed(hr, "Failed to create object data buffer!");

	void* mappedObjectData = nullptr;

	D3D12_RANGE objectReadRange{};
	hr = m_objectDataBuffer->Map(0, &objectReadRange, &mappedObjectData);

	memcpy(mappedObjectData, objects.data(), objectCount * sizeof(ObjectData));

	m_objectDataBuffer->Unmap(0, nullptr);
}

void Renderer::CreateIndirectBuffers()
{
	// Compute appends commands here, while ExecuteIndirect consumes the same buffer later in the frame.
	UINT64 bufferSize = sizeof(IndirectCommand) * objectCount;

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC bufferDesc{};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = bufferSize;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	HRESULT hr = m_device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_indirectArgsBuffer)
	);

	ThrowIfFailed(hr, "Failed to create indirect argument buffer");

	bufferDesc.Width = sizeof(uint32_t);
	hr = m_device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_indirectCountBuffer));
	ThrowIfFailed(hr, "Failed to create indirect count buffer");
}

void Renderer::CreateDescriptorHeap()
{
	// Slots 0-3 are CBV, object SRV, command UAV, and count UAV respectively.
	D3D12_DESCRIPTOR_HEAP_DESC objectHeapDesc{};
	objectHeapDesc.NumDescriptors = 6;
	objectHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	objectHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	HRESULT hr = m_device->CreateDescriptorHeap(
		&objectHeapDesc,
		IID_PPV_ARGS(&m_srvHeap)
	);

	ThrowIfFailed(hr, "Failed to create object data descriptor heap!");

	D3D12_DESCRIPTOR_HEAP_DESC clearHeapDesc{};
	clearHeapDesc.NumDescriptors = 1;
	clearHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	clearHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&clearHeapDesc, IID_PPV_ARGS(&m_clearHeap)),
		"Create CPU clear descriptor heap");

	m_srvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void Renderer::CreateCameraCBV()
{
	// Create camera CBV in descriptor 0
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	cbvDesc.BufferLocation = m_cameraBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = 256;

	auto cpuHandle = m_srvHeap->GetCPUDescriptorHandleForHeapStart();

	m_device->CreateConstantBufferView(&cbvDesc, cpuHandle);
}

void Renderer::CreateObjectSRV()
{
	// The descriptors below must match the register bindings in Triangle.hlsl and Culling.hlsl.
	// Create shader resource view
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Buffer.NumElements = objectCount;
	srvDesc.Buffer.StructureByteStride = sizeof(ObjectData);

	// Creates SRV at descriptor 1
	auto cpuHandle = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
	UINT descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cpuHandle.ptr += descriptorSize;

	m_device->CreateShaderResourceView(m_objectDataBuffer.Get(), &srvDesc, cpuHandle);

	// Create indirect-command UAV at descriptor 2.
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = objectCount;
	uavDesc.Buffer.StructureByteStride = sizeof(IndirectCommand);
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	cpuHandle.ptr += m_srvDescriptorSize;

	m_device->CreateUnorderedAccessView(
		m_indirectArgsBuffer.Get(),
		nullptr,
		&uavDesc,
		cpuHandle
	);

	// Create the command-count UAV at descriptor 3.
	cpuHandle.ptr += m_srvDescriptorSize;
	D3D12_UNORDERED_ACCESS_VIEW_DESC countUavDesc = {};
	countUavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	countUavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	countUavDesc.Buffer.NumElements = 1;
	countUavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
	m_device->CreateUnorderedAccessView(m_indirectCountBuffer.Get(), nullptr, &countUavDesc, cpuHandle);
	m_device->CreateUnorderedAccessView(m_indirectCountBuffer.Get(), nullptr, &countUavDesc,
		m_clearHeap->GetCPUDescriptorHandleForHeapStart());
}

void Renderer::CreateRootSignature()
{
	// Graphics root parameters are: camera CBV, object SRV table, and per-command object index constant.
	// Create root signature
	D3D12_DESCRIPTOR_RANGE range{};
	range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range.NumDescriptors = 1;
	range.BaseShaderRegister = 0;
	range.RegisterSpace = 0;
	range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// Camera CBV
	D3D12_ROOT_PARAMETER cameraParameter{};
	cameraParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	cameraParameter.Descriptor.ShaderRegister = 0;
	cameraParameter.Descriptor.RegisterSpace = 0;
	cameraParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// Object SRV
	D3D12_ROOT_PARAMETER objectParameter{};
	objectParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	objectParameter.DescriptorTable.NumDescriptorRanges = 1;
	objectParameter.DescriptorTable.pDescriptorRanges = &range;
	objectParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	D3D12_ROOT_PARAMETER objectIndexParameter{};
	objectIndexParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	objectIndexParameter.Constants.ShaderRegister = 1;
	objectIndexParameter.Constants.RegisterSpace = 0;
	objectIndexParameter.Constants.Num32BitValues = 1;
	objectIndexParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	D3D12_ROOT_PARAMETER parameter[3] =
	{
		cameraParameter,
		objectParameter,
		objectIndexParameter
	};

	// Create root signature
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.NumParameters = 3;
	rootSignatureDesc.pParameters = parameter;
	rootSignatureDesc.NumStaticSamplers = 0;
	rootSignatureDesc.pStaticSamplers = nullptr;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	Microsoft::WRL::ComPtr<ID3DBlob> signature;
	Microsoft::WRL::ComPtr<ID3DBlob> error;

	HRESULT hr = D3D12SerializeRootSignature(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&signature,
		&error
	);

	ThrowIfFailedDetailed(hr, error, "Failed to serialize root signature!");

	hr = m_device->CreateRootSignature(
		0,
		signature->GetBufferPointer(),
		signature->GetBufferSize(),
		IID_PPV_ARGS(&m_rootSignature)
	);

	ThrowIfFailed(hr, "Failed to create root signature!");
}

void Renderer::CreatePipelineState()
{
	// The PSO combines compiled shaders with the fixed-function state used by the cube pass.
	// Create graphics pipeline state object (PSO)
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
	psoDesc.pRootSignature = m_rootSignature.Get();
	psoDesc.VS = { m_vertexShader->GetBufferPointer(), m_vertexShader->GetBufferSize() };
	psoDesc.PS = { m_pixelShader->GetBufferPointer(), m_pixelShader->GetBufferSize() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// Set up the rasterizer state
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.SlopeScaledDepthBias = 0.0f;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.AntialiasedLineEnable = FALSE;
	rasterizerDesc.ForcedSampleCount = 0;
	rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	psoDesc.RasterizerState = rasterizerDesc;

	// Set up the default blend state (opaque rendering, no blending).
	D3D12_BLEND_DESC blendDesc{};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	for (D3D12_RENDER_TARGET_BLEND_DESC& renderTarget : blendDesc.RenderTarget)
	{
		renderTarget.BlendEnable = FALSE;
		renderTarget.LogicOpEnable = FALSE;
		renderTarget.SrcBlend = D3D12_BLEND_ONE;
		renderTarget.DestBlend = D3D12_BLEND_ZERO;
		renderTarget.BlendOp = D3D12_BLEND_OP_ADD;
		renderTarget.SrcBlendAlpha = D3D12_BLEND_ONE;
		renderTarget.DestBlendAlpha = D3D12_BLEND_ZERO;
		renderTarget.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		renderTarget.LogicOp = D3D12_LOGIC_OP_NOOP;
		renderTarget.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	}
	psoDesc.BlendState = blendDesc;

	// Setup the depth-stencil state.
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	depthStencilDesc.StencilEnable = FALSE;
	psoDesc.DepthStencilState = depthStencilDesc;

	// Set up the render target formats
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleMask = UINT_MAX;

	// Create the graphics pipeline state object (PSO)
	HRESULT hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));

	ThrowIfFailed(hr, "Failed to create graphics pipeline state!");
}

void Renderer::CreateCullingRootSignature()
{
	// Compute bindings are camera b0, objects t0, commands u0, and the append counter u1.
	D3D12_ROOT_PARAMETER rootParameters[4]{};

	// Parameter 0: Camera CBV (b0)
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].Descriptor.ShaderRegister = 0;
	rootParameters[0].Descriptor.RegisterSpace = 0;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// Parameter 1: Object SRV (t0)
	D3D12_DESCRIPTOR_RANGE objectRange{};
	objectRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	objectRange.NumDescriptors = 1;
	objectRange.BaseShaderRegister = 0;
	objectRange.RegisterSpace = 0;
	objectRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[1].DescriptorTable.pDescriptorRanges = &objectRange;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// Parameter 2: Indirect command UAV (u0) and count UAV (u1)
	D3D12_DESCRIPTOR_RANGE visibilityRange{};
	visibilityRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	visibilityRange.NumDescriptors = 1;
	visibilityRange.BaseShaderRegister = 0;
	visibilityRange.RegisterSpace = 0;
	visibilityRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[2].DescriptorTable.pDescriptorRanges = &visibilityRange;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
	rootParameters[3].Descriptor.ShaderRegister = 1;
	rootParameters[3].Descriptor.RegisterSpace = 0;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootSignatureDesc.NumParameters = 4;
	rootSignatureDesc.pParameters = rootParameters;
	rootSignatureDesc.NumStaticSamplers = 0;
	rootSignatureDesc.pStaticSamplers = nullptr;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

	HRESULT hr = D3D12SerializeRootSignature(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&signatureBlob,
		&errorBlob
	);

	ThrowIfFailedDetailed(hr, errorBlob, "Failed to serialize culling root signature!");

	hr = m_device->CreateRootSignature(
		0,
		signatureBlob->GetBufferPointer(),
		signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&m_cullingRootSignature)
	);

	ThrowIfFailed(hr, "Failed to create culling root signature!");
	assert(m_cullingRootSignature);

}

void Renderer::CreateCullingPipeline()
{
	// Compile the compute shader at startup so shader errors are reported before frame recording begins.
	HRESULT hr = D3DCompileFromFile(
		L"shaders/Culling.hlsl",
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"CSMain",
		"cs_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS,
		0,
		&m_cullingShader,
		nullptr
	);

	ThrowIfFailed(hr, "Failed to compile compute shader!");

	D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};
	desc.pRootSignature = m_cullingRootSignature.Get();
	desc.CS.pShaderBytecode = m_cullingShader->GetBufferPointer();
	desc.CS.BytecodeLength = m_cullingShader->GetBufferSize();

	hr = m_device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&m_cullingPipelineState));

	ThrowIfFailed(hr, "Create compute pipeline state failed");
	assert(m_cullingPipelineState);
}

void Renderer::CreateCommandSignature()
{
	// Each command sets the object-index root constant, then executes one indexed draw.
	D3D12_INDIRECT_ARGUMENT_DESC arguments[2]{};
	arguments[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
	arguments[0].Constant.RootParameterIndex = 2;
	arguments[0].Constant.DestOffsetIn32BitValues = 0;
	arguments[0].Constant.Num32BitValuesToSet = 1;
	arguments[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

	D3D12_COMMAND_SIGNATURE_DESC desc{};
	desc.ByteStride = sizeof(IndirectCommand);
	desc.NumArgumentDescs = _countof(arguments);
	desc.pArgumentDescs = arguments;
	ThrowIfFailed(m_device->CreateCommandSignature(&desc, m_rootSignature.Get(), IID_PPV_ARGS(&m_commandSignature)),
		"Create indirect command signature");
}

void Renderer::DispatchCulling()
{
	// This pass converts object data into a compact command stream for ExecuteIndirect.
	m_commandList->SetComputeRootSignature(m_cullingRootSignature.Get());

	ID3D12DescriptorHeap* heaps[] = { m_srvHeap.Get() };

	m_commandList->SetDescriptorHeaps(1, heaps);

	// b0 - camera
	m_commandList->SetComputeRootConstantBufferView(0, m_cameraBuffer->GetGPUVirtualAddress());

	// t0 - object buffer
	D3D12_GPU_DESCRIPTOR_HANDLE objectHandle = m_srvHeap->GetGPUDescriptorHandleForHeapStart();

	objectHandle.ptr += m_srvDescriptorSize;

	m_commandList->SetComputeRootDescriptorTable(1, objectHandle);

	// u0 is the command UAV; u1 is the root-bound append counter.
	D3D12_GPU_DESCRIPTOR_HANDLE commandHandle = m_srvHeap->GetGPUDescriptorHandleForHeapStart();
	commandHandle.ptr += m_srvDescriptorSize * 2;
	m_commandList->SetComputeRootDescriptorTable(2, commandHandle);
	m_commandList->SetComputeRootUnorderedAccessView(3, m_indirectCountBuffer->GetGPUVirtualAddress());

	m_commandList->SetPipelineState(m_cullingPipelineState.Get());

	// Buffers are COMMON on creation, then remain INDIRECT_ARGUMENT between frames.
	D3D12_RESOURCE_BARRIER stateBarriers[2]{};
	const D3D12_RESOURCE_STATES generatedBufferState =
		m_indirectBuffersInitialized ? D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT : D3D12_RESOURCE_STATE_COMMON;
	for (auto& barrier : stateBarriers)
	{
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.StateBefore = generatedBufferState;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	}
	stateBarriers[0].Transition.pResource = m_indirectArgsBuffer.Get();
	stateBarriers[1].Transition.pResource = m_indirectCountBuffer.Get();
	m_commandList->ResourceBarrier(2, stateBarriers);
	m_indirectBuffersInitialized = true;

	D3D12_RESOURCE_BARRIER barriers[2]{};
	barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barriers[0].UAV.pResource = m_indirectArgsBuffer.Get();
	barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barriers[1].UAV.pResource = m_indirectCountBuffer.Get();
	m_commandList->ResourceBarrier(2, barriers);

	// Clear only the counter. The structured command buffer is not compatible with UAV clear operations.
	D3D12_GPU_DESCRIPTOR_HANDLE countGpu = m_srvHeap->GetGPUDescriptorHandleForHeapStart();
	countGpu.ptr += m_srvDescriptorSize * 3;
	D3D12_CPU_DESCRIPTOR_HANDLE countCpu = m_clearHeap->GetCPUDescriptorHandleForHeapStart();
	UINT zeroes[4] = {};
	m_commandList->ClearUnorderedAccessViewUint(countGpu, countCpu, m_indirectCountBuffer.Get(), zeroes, 0, nullptr);

	m_commandList->Dispatch((objectCount + cullingThreadGroupSize - 1) / cullingThreadGroupSize, 1, 1);

	// Ensure all atomic appends are visible before the graphics queue interprets the buffer as commands.
	D3D12_RESOURCE_BARRIER computeBarrier[2]{};
	computeBarrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	computeBarrier[0].UAV.pResource = m_indirectArgsBuffer.Get();
	computeBarrier[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	computeBarrier[1].UAV.pResource = m_indirectCountBuffer.Get();
	m_commandList->ResourceBarrier(2, computeBarrier);

	D3D12_RESOURCE_BARRIER generatedBarriers[2]{};
	for (auto& barrier : generatedBarriers)
	{
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	}
	generatedBarriers[0].Transition.pResource = m_indirectArgsBuffer.Get();
	generatedBarriers[1].Transition.pResource = m_indirectCountBuffer.Get();
	m_commandList->ResourceBarrier(2, generatedBarriers);
}
