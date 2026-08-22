#include "Renderer.h"

#include <stdexcept>

void Renderer::Initialize(HWND hwnd)
{
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

	CreateDescriptorHeap();
	CreateCameraCBV();
	CreateObjectSRV();
	CreateRootSignature();
	CreatePipelineState();
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
	HRESULT hr = m_commandAllocator->Reset();
	if (FAILED(hr))
		throw std::runtime_error("Failed to reset command allocator!");

	hr = m_commandList->Reset(m_commandAllocator.Get(), nullptr);
	if (FAILED(hr))
		throw std::runtime_error("Failed to reset command list!");

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
	viewport.Width = 1280.0f;
	viewport.Height = 720.0f;
	viewport.MaxDepth = 1.0f;
	m_commandList->RSSetViewports(1, &viewport);

	D3D12_RECT scissorRect{ 0, 0, 1280, 720 };
	m_commandList->RSSetScissorRects(1, &scissorRect);

	// Clear the render target
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
}

void Renderer::Render()
{
	// Set the root signature, pipeline state, and draw the triangle
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	ID3D12DescriptorHeap* descriptorHeaps[] = { m_srvheap.Get() };

	m_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	// Bind camera CBV (root parameter 0)
	m_commandList->SetGraphicsRootConstantBufferView(0, m_cameraBuffer->GetGPUVirtualAddress());

	// Get descriptor 1 (object buffer SRV)
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = m_srvheap->GetGPUDescriptorHandleForHeapStart();
	gpuHandle.ptr += m_srvDescriptorSize;

	// Bind object buffer SRV (root parameter 1)
	m_commandList->SetGraphicsRootDescriptorTable(1, gpuHandle);

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
	if (FAILED(hr))
		throw std::runtime_error("Failed to close command list!");

	ID3D12CommandList* commandLists[] = { m_commandList.Get() };

	m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	hr = m_swapChain->Present(1, 0);

	if (FAILED(hr))
		throw std::runtime_error("Failed to present swap chain!");
	
	WaitForPreviousFrame();

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void Renderer::CreateDevice(UINT flags)
{
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
}

void Renderer::CreateCommandObjects()
{
	// Create command allocator and command list
	HRESULT hr = m_device->CreateCommandAllocator(
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
		1280.0f / 720.0f,
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

	if (FAILED(hr))
		throw std::runtime_error("Create camera buffer failed");

	// Upload the camera
	void* mappedData = nullptr;

	D3D12_RANGE readRange{};

	hr = m_cameraBuffer->Map(0, &readRange, &mappedData);

	if (FAILED(hr))
		throw std::runtime_error("Map camera buffer failed");

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

	if (FAILED(hr))
		throw std::runtime_error("Failed to create vertex buffer!");

	// Copy vertex data to the buffer
	void* mappedData = nullptr;

	D3D12_RANGE readRange{};

	hr = m_vertexBuffer->Map(0, &readRange, &mappedData);

	if (FAILED(hr))
		throw std::runtime_error("Failed to map vertex buffer!");

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

	if (FAILED(hr))
		throw std::runtime_error("Failed to create index buffer!");

	// Copy index data to the buffer
	hr = m_indexBuffer->Map(0, &readRange, &mappedData);

	if (FAILED(hr))
		throw std::runtime_error("Failed to map index buffer!");

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

	if (FAILED(hr))
		throw std::runtime_error("Failed to compile vertex shader!");

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

	if (FAILED(hr))
		throw std::runtime_error("Failed to compile pixel shader!");

	// Create object data buffer
	std::vector<ObjectData> objects;
	objects.resize(ObjectCount);

	for (uint32_t i = 0; i < ObjectCount; ++i)
	{
		uint32_t x = i % 32;
		uint32_t y = i / 32;

		DirectX::XMMATRIX translation = DirectX::XMMatrixTranslation
		(
			(x - 16.0f) * 1.5f,
			(y - 16.0f) * 1.5f,
			0.0f
		);
		DirectX::XMStoreFloat4x4(&objects[i].modelMatrix, DirectX::XMMatrixTranspose(translation));
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

	if (FAILED(hr))
		throw std::runtime_error("Failed to create object data buffer!");

	void* mappedObjectData = nullptr;

	D3D12_RANGE objectReadRange{};
	hr = m_objectDataBuffer->Map(0, &objectReadRange, &mappedObjectData);

	memcpy(mappedObjectData, objects.data(), ObjectCount * sizeof(ObjectData));

	m_objectDataBuffer->Unmap(0, nullptr);
}

void Renderer::CreateDescriptorHeap()
{
	// Create descriptor heap for object data
	D3D12_DESCRIPTOR_HEAP_DESC objectHeapDesc{};
	objectHeapDesc.NumDescriptors = 2;
	objectHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	objectHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	HRESULT hr = m_device->CreateDescriptorHeap(
		&objectHeapDesc,
		IID_PPV_ARGS(&m_srvheap)
	);

	if (FAILED(hr))
		throw std::runtime_error("Failed to create object data descriptor heap!");

	m_srvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void Renderer::CreateCameraCBV()
{
	// Create camera CBV in descriptor 0
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	cbvDesc.BufferLocation = m_cameraBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = 256;

	auto cpuHandle = m_srvheap->GetCPUDescriptorHandleForHeapStart();

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
	auto cpuHandle = m_srvheap->GetCPUDescriptorHandleForHeapStart();
	UINT descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cpuHandle.ptr += descriptorSize;

	m_device->CreateShaderResourceView(m_objectDataBuffer.Get(), &srvDesc, cpuHandle);
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

	D3D12_ROOT_PARAMETER parameter[2] =
	{
		cameraParameter,
		objectParameter
	};

	// Create root signature
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.NumParameters = 2;
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

	if (FAILED(hr))
	{
		if (error)
		{
			std::string errorMessage(static_cast<const char*>(error->GetBufferPointer()), error->GetBufferSize());
			throw std::runtime_error("Failed to serialize root signature: " + errorMessage);
		}
		else
		{
			throw std::runtime_error("Failed to serialize root signature!");
		}
	}

	hr = m_device->CreateRootSignature(
		0,
		signature->GetBufferPointer(),
		signature->GetBufferSize(),
		IID_PPV_ARGS(&m_rootSignature)
	);

	if (FAILED(hr))
	{
		if (error)
		{
			std::string errorMessage(static_cast<const char*>(error->GetBufferPointer()), error->GetBufferSize());
			throw std::runtime_error("Failed to create root signature: " + errorMessage);
		}
		else
		{
			throw std::runtime_error("Failed to create root signature!");
		}
	}
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

	if (FAILED(hr))
	{
		char buffer[256];

		sprintf_s(
			buffer,
			"CreateGraphicsPipelineState failed: 0x%08X\n",
			static_cast<unsigned>(hr));

		OutputDebugStringA(buffer);

		throw std::runtime_error(
			"CreateGraphicsPipelineState failed");
	}
}
