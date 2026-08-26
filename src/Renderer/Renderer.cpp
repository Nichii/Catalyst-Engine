#include "Renderer.h"
#include "../Core/Dx12Utils.h"

#include <stdexcept>
#include <d3dcompiler.h>

Renderer::~Renderer()
{
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
	
	CreateDevice(flags);
	CreateSwapChain(hwnd);
	CreateCommandObjects();
	CreateRenderTargets();

	CreateCameraBuffer();
	CreateObjectBuffer();
	CreateVisibleObjectBuffer();

	CreateDescriptorHeap();
	CreateCameraCBV();
	CreateObjectSRV();
	CreateRootSignature();
	CreatePipelineState();

	CreateCullingRootSignature();
	CreateCullingPipeline();
}

void Renderer::WaitForPreviousFrame()
{
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
	// Reset command allocator and command list for the current frame
	HRESULT hr = m_commandAllocator->Reset();
	ThrowIfFailed(hr, "Reset command allocator");

	hr = m_commandList->Reset(m_commandAllocator.Get(), nullptr);
	ThrowIfFailed(hr, "Reset command list");

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

	// Set the viewport and scissor rectangle used for rasterization.
	D3D12_VIEWPORT viewport{};
	viewport.Width = static_cast<float>(defaultWidth);
	viewport.Height = static_cast<float>(defaultHeight);
	viewport.MaxDepth = 1.0f;
	m_commandList->RSSetViewports(1, &viewport);

	D3D12_RECT scissorRect{ 0L, 0L, static_cast<LONG>(defaultWidth), static_cast<LONG>(defaultHeight) };
	m_commandList->RSSetScissorRects(1, &scissorRect);

	// Clear the render target
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
}

void Renderer::Render()
{
	DispatchCulling();

	// Set the root signature, pipeline state, and draw visible cube instances.
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	ID3D12DescriptorHeap* descriptorHeaps[] = { m_srvHeap.Get() };

	m_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	// Bind camera CBV (root parameter 0)
	m_commandList->SetGraphicsRootConstantBufferView(0, m_cameraBuffer->GetGPUVirtualAddress());

	// Get descriptor 1 (object buffer SRV)
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_srvHeap->GetGPUDescriptorHandleForHeapStart();
	gpuHandle.ptr += m_srvDescriptorSize;

	// Bind object buffer SRV (root parameter 1)
	m_commandList->SetGraphicsRootDescriptorTable(1, gpuHandle);

	// Bind GPU visibility mask SRV (root parameter 2)
	gpuHandle.ptr += m_srvDescriptorSize;
	m_commandList->SetGraphicsRootDescriptorTable(2, gpuHandle);

	m_commandList->SetPipelineState(m_pipelineState.Get());
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->IASetIndexBuffer(&m_indexBufferView);
	m_commandList->DrawIndexedInstanced(36, ObjectCount, 0, 0, 0);
}

void Renderer::EndFrame()
{
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

	D3D12_RANGE readRange{ 0, sizeof(uint32_t) * ObjectCount };

	void* mappedData = nullptr;
	hr = m_visibilityReadbackBuffer->Map(0, &readRange, &mappedData);

	ThrowIfFailed(hr, "Failed to map visibility readback buffer!");

	const uint32_t* visibility = static_cast<const uint32_t*>(mappedData);
	m_visibleObjectCount = 0;
	for (uint32_t i = 0; i < ObjectCount; ++i)
		m_visibleObjectCount += visibility[i] != 0 ? 1u : 0u;

	D3D12_RANGE writtenRange{ 0, 0 };
	m_visibilityReadbackBuffer->Unmap(0, &writtenRange);

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

	// Create fence and fence event
	hr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));

	ThrowIfFailed(hr, "Failed to create fence!");

	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	ThrowIfFailed(hr, "Failed to create fence event!");
}

void Renderer::CreateCameraBuffer()
{
	// Create camera matrix
	DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH
	(
		DirectX::XMVectorSet(0.0f, 0.0f, -50.0f, 1.0f),
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

void Renderer::CreateObjectBuffer()
{
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
	objects.resize(ObjectCount);

	for (uint32_t i = 0; i < ObjectCount; ++i)
	{
		ObjectData object{};

		float x = (i % 32 - 16) * 0.75f;
		float y = (i / (float)32 - 16) * 0.75f;

		// World transform
		DirectX::XMMATRIX world = DirectX::XMMatrixTranslation(x, y, 0.0f);

		DirectX::XMStoreFloat4x4(&object.worldMatrix, DirectX::XMMatrixTranspose(world));

		// Bounding sphere
		object.bounds = { x, y, 0.0f, 0.866f };

		objects[i] = object;
	}

	D3D12_HEAP_PROPERTIES objectHeap{};
	objectHeap.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC objectBufferDesc{};
	objectBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	objectBufferDesc.Alignment = 0;
	objectBufferDesc.Width = ObjectCount * sizeof(ObjectData);
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

	memcpy(mappedObjectData, objects.data(), ObjectCount * sizeof(ObjectData));

	m_objectDataBuffer->Unmap(0, nullptr);
}

void Renderer::CreateVisibleObjectBuffer()
{
	UINT64 bufferSize = sizeof(uint32_t) * ObjectCount;

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
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_visibleObjectBuffer)
	);

	ThrowIfFailed(hr, "Failed to create visible object buffer");
}

void Renderer::CreateDescriptorHeap()
{
	// Create descriptor heap for object data
	D3D12_DESCRIPTOR_HEAP_DESC objectHeapDesc{};
	objectHeapDesc.NumDescriptors = 4;
	objectHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	objectHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	HRESULT hr = m_device->CreateDescriptorHeap(
		&objectHeapDesc,
		IID_PPV_ARGS(&m_srvHeap)
	);

	ThrowIfFailed(hr, "Failed to create object data descriptor heap!");

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
	// Create shader resource view
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Buffer.NumElements = ObjectCount;
	srvDesc.Buffer.StructureByteStride = sizeof(ObjectData);

	// Creates SRV at descriptor 1
	auto cpuHandle = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
	UINT descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cpuHandle.ptr += descriptorSize;

	m_device->CreateShaderResourceView(m_objectDataBuffer.Get(), &srvDesc, cpuHandle);

	// Create visibility SRV at descriptor 2.
	cpuHandle.ptr += m_srvDescriptorSize;
	D3D12_SHADER_RESOURCE_VIEW_DESC visibilitySrvDesc{};
	visibilitySrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	visibilitySrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	visibilitySrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	visibilitySrvDesc.Buffer.NumElements = ObjectCount;
	visibilitySrvDesc.Buffer.StructureByteStride = sizeof(uint32_t);
	m_device->CreateShaderResourceView(m_visibleObjectBuffer.Get(), &visibilitySrvDesc, cpuHandle);

	// Create UAV at descriptor 3.
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = ObjectCount;
	uavDesc.Buffer.StructureByteStride = sizeof(uint32_t);
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	cpuHandle.ptr += m_srvDescriptorSize;

	m_device->CreateUnorderedAccessView(
		m_visibleObjectBuffer.Get(),
		nullptr,
		&uavDesc,
		cpuHandle
	);
}

void Renderer::CreateRootSignature()
{
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

	D3D12_DESCRIPTOR_RANGE visibilityRange{};
	visibilityRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	visibilityRange.NumDescriptors = 1;
	visibilityRange.BaseShaderRegister = 1;
	visibilityRange.RegisterSpace = 0;
	visibilityRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER visibilityParameter{};
	visibilityParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	visibilityParameter.DescriptorTable.NumDescriptorRanges = 1;
	visibilityParameter.DescriptorTable.pDescriptorRanges = &visibilityRange;
	visibilityParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	D3D12_ROOT_PARAMETER parameter[3] =
	{
		cameraParameter,
		objectParameter,
		visibilityParameter
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

	// Setup the depth-stencil state (no depth testing or writing).
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = FALSE;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	depthStencilDesc.StencilEnable = FALSE;
	psoDesc.DepthStencilState = depthStencilDesc;

	// Set up the render target formats
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleMask = UINT_MAX;

	// Create the graphics pipeline state object (PSO)
	HRESULT hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));

	ThrowIfFailed(hr, "Failed to create graphics pipeline state!");
}

void Renderer::CreateCullingRootSignature()
{
	D3D12_ROOT_PARAMETER rootParameters[3]{};

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

	// Parameter 2: Visibility UAV (u0)
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
	rootSignatureDesc.NumParameters = 3;
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

	// Create visibility readback buffer
	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_READBACK;

	D3D12_RESOURCE_DESC bufferDesc{};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = sizeof(uint32_t) * ObjectCount;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	hr = m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_visibilityReadbackBuffer));

	ThrowIfFailed(hr, "Failed to create visibility readback buffer!");
	assert(m_visibilityReadbackBuffer);
}

void Renderer::CreateCullingPipeline()
{
	// Load/compile Culling.hlsl first
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

void Renderer::DispatchCulling()
{
	m_commandList->SetComputeRootSignature(m_cullingRootSignature.Get());

	ID3D12DescriptorHeap* heaps[] = { m_srvHeap.Get() };

	m_commandList->SetDescriptorHeaps(1, heaps);

	// b0 - camera
	m_commandList->SetComputeRootConstantBufferView(0, m_cameraBuffer->GetGPUVirtualAddress());

	// t0 - object buffer
	D3D12_GPU_DESCRIPTOR_HANDLE objectHandle = m_srvHeap->GetGPUDescriptorHandleForHeapStart();

	objectHandle.ptr += m_srvDescriptorSize;

	m_commandList->SetComputeRootDescriptorTable(1, objectHandle);

	// u0 - visibility buffer
	D3D12_GPU_DESCRIPTOR_HANDLE visibilityHandle = m_srvHeap->GetGPUDescriptorHandleForHeapStart();

	visibilityHandle.ptr += m_srvDescriptorSize * 3;

	m_commandList->SetComputeRootDescriptorTable(2, visibilityHandle);

	m_commandList->SetPipelineState(m_cullingPipelineState.Get());

	// Transition visible object buffer to UAV
	D3D12_RESOURCE_BARRIER barrierCommonToUAV{};
	barrierCommonToUAV.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierCommonToUAV.Transition.pResource = m_visibleObjectBuffer.Get();
	barrierCommonToUAV.Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;
	barrierCommonToUAV.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	barrierCommonToUAV.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_commandList->ResourceBarrier(1, &barrierCommonToUAV);

	m_commandList->Dispatch((ObjectCount + cullingThreadGroupSize - 1) / cullingThreadGroupSize, 1, 1);

	// Compute -> Graphics barrier
	D3D12_RESOURCE_BARRIER barrierUAV{};
	barrierUAV.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrierUAV.UAV.pResource = m_visibleObjectBuffer.Get();

	m_commandList->ResourceBarrier(1, &barrierUAV);

	// Transition visibility buffer
	D3D12_RESOURCE_BARRIER toCopy{};
	toCopy.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	toCopy.Transition.pResource = m_visibleObjectBuffer.Get();
	toCopy.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	toCopy.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
	toCopy.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_commandList->ResourceBarrier(1, &toCopy);

	assert(m_visibilityReadbackBuffer);
	assert(m_visibleObjectBuffer);

	if (!m_visibilityReadbackBuffer)
		throw std::runtime_error("Readback buffer is NULL");

	if (!m_visibleObjectBuffer)
		throw std::runtime_error("Visibility buffer is NULL");

	m_commandList->CopyResource(m_visibilityReadbackBuffer.Get(), m_visibleObjectBuffer.Get());

	// Leave the visibility buffer shader-readable for the graphics pass and the next frame's culling pass.
	D3D12_RESOURCE_BARRIER backToShaderReadable{};
	backToShaderReadable.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	backToShaderReadable.Transition.pResource = m_visibleObjectBuffer.Get();
	backToShaderReadable.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
	backToShaderReadable.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	backToShaderReadable.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_commandList->ResourceBarrier(1, &backToShaderReadable);
}
