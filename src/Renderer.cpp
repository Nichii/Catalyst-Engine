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

	// Close the command list as it will be reset before recording commands
	m_commandList->Close();

	// Create triangle vertices
	const Vertex triangleVertices[] =
	{
		{ { 0.0f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
		{ { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
		{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
	};

	const UINT triangleVertexBufferSize = sizeof(triangleVertices);

	// Create vertex buffer
	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC bufferDesc{};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Alignment = 0;
	bufferDesc.Width = triangleVertexBufferSize;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.SampleDesc.Quality = 0;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	hr = m_device->CreateCommittedResource(
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
	readRange.Begin = 0;
	readRange.End = 0;

	hr = m_vertexBuffer->Map(0, &readRange, &mappedData);

	if (FAILED(hr))
		throw std::runtime_error("Failed to map vertex buffer!");

	memcpy(mappedData, triangleVertices, triangleVertexBufferSize);
	m_vertexBuffer->Unmap(0, nullptr);

	// Create vertex buffer view
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = triangleVertexBufferSize;

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

	// Create root signature
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.NumParameters = 0;
	rootSignatureDesc.pParameters = nullptr;
	rootSignatureDesc.NumStaticSamplers = 0;
	rootSignatureDesc.pStaticSamplers = nullptr;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	Microsoft::WRL::ComPtr<ID3DBlob> signature;
	Microsoft::WRL::ComPtr<ID3DBlob> error;

	hr = D3D12SerializeRootSignature(
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

	// Define input layout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};


	// Create graphics pipeline state object (PSO)
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
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
	hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));

	if (FAILED(hr))
		throw std::runtime_error("Failed to create graphics pipeline state object!");
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

	// Set the viewport and scissor rectangle used for rasterization.
	D3D12_VIEWPORT viewport{};
	viewport.Width = 1280.0f;
	viewport.Height = 720.0f;
	viewport.MaxDepth = 1.0f;
	m_commandList->RSSetViewports(1, &viewport);

	D3D12_RECT scissorRect{ 0, 0, 1280, 720 };
	m_commandList->RSSetScissorRects(1, &scissorRect);

	// Clear the render target
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f }; // Temporary clear color
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// Set the root signature, pipeline state, and draw the triangle
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	m_commandList->SetPipelineState(m_pipelineState.Get());
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->DrawInstanced(3, 1, 0, 0);

	// Transition the render target back before submitting the command list.
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	m_commandList->ResourceBarrier(1, &barrier);
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
}
