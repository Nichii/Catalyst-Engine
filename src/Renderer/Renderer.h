#pragma once

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <vector>
#include <chrono>

struct Vertex
{
	// There is no material data yet; position is enough to draw the test cubes.
	DirectX::XMFLOAT3 position;
};

struct ObjectData
{
	// Keep this layout in sync with ObjectData in the shaders.
	DirectX::XMFLOAT4X4 worldMatrix;
	// xyz is the sphere center and w is its radius.
	DirectX::XMFLOAT4 bounds;
};

struct CameraData
{
	// The matrix is transposed before it is copied to the upload buffer.
	DirectX::XMFLOAT4X4 viewProjection;
};

struct VisibleObject
{
	uint32_t objectID;
};

struct IndirectCommand
{
	// ExecuteIndirect writes this value before the indexed-draw fields are used.
	uint32_t objectID;
	D3D12_DRAW_INDEXED_ARGUMENTS draw;
};

class Renderer
{
public:
	static constexpr UINT defaultWidth = 1280;
	static constexpr UINT defaultHeight = 720;

	~Renderer();
	Renderer& operator=(const Renderer&) = delete;

	void Initialize(HWND hwnd);
	void WaitForPreviousFrame();
	void BeginFrame();
	void Render();
	void EndFrame();

private:
	static constexpr UINT bufferCount = 2;
	static constexpr UINT cullingThreadGroupSize = 64;
	// Size the buffer for the worst case where every object is visible.
	static constexpr uint32_t ObjectCount = 10000;

	static constexpr float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f }; // Temporary clear color

	UINT m_rtvDescriptorSize = 0;
	UINT m_srvDescriptorSize = 0;
	UINT m_frameIndex = 0;
	UINT m_width = defaultWidth;
	UINT m_height = defaultHeight;
	HWND m_hwnd = nullptr;
	UINT64 m_fenceValue = 0;
	HANDLE m_fenceEvent = nullptr;

	Microsoft::WRL::ComPtr<IDXGIFactory7> m_factory;
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[bufferCount];
	Microsoft::WRL::ComPtr<ID3D12Resource> m_depthStencil;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView{};

	Microsoft::WRL::ComPtr<ID3DBlob> m_vertexShader;
	Microsoft::WRL::ComPtr<ID3DBlob> m_pixelShader;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	// The shader-visible descriptors used by the root signatures live here.
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;
	// This matching CPU-only descriptor is used when clearing the count buffer.
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_clearHeap;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView{};

	Microsoft::WRL::ComPtr<ID3D12Resource> m_objectDataBuffer;
	// This member started as the visibility buffer and now holds compact commands.
	Microsoft::WRL::ComPtr<ID3D12Resource> m_visibleObjectBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indirectArgsBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indirectCountBuffer;

	Microsoft::WRL::ComPtr<ID3DBlob> m_cullingShader;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_cullingRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_cullingPipelineState;
	Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_commandSignature;
	bool m_useIndirect = true;
	bool m_indirectBuffersInitialized = false;
	uint64_t m_frameCounter = 0;
	std::chrono::steady_clock::time_point m_lastDiagnostic = std::chrono::steady_clock::now();

	Microsoft::WRL::ComPtr<ID3D12Resource> m_cameraBuffer;
	CameraData m_cameraData{};

	void CreateDevice(UINT flags);
	void CreateSwapChain(HWND hwnd);
	void CreateCommandObjects();
	void CreateRenderTargets();
	void Resize(UINT width, UINT height);
	void UpdateCamera();
	void CreateDepthStencil();

	void CreateCameraBuffer();
	void CreateObjectBuffer();
	void CreateVisibleObjectBuffer();

	void CreateDescriptorHeap();
	void CreateCameraCBV();
	void CreateObjectSRV();
	void CreateRootSignature();
	void CreatePipelineState();

	void CreateCullingRootSignature();
	void CreateCullingPipeline();
	void CreateCommandSignature();
	void DispatchCulling();

	// Unit cube geometry shared by every instance.
	static constexpr Vertex vertices[] =
	{
		{ {-0.5f, -0.5f, -0.5f} },
		{ {-0.5f,  0.5f, -0.5f} },
		{ { 0.5f,  0.5f, -0.5f} },
		{ { 0.5f, -0.5f, -0.5f} },

		{ {-0.5f, -0.5f,  0.5f} },
		{ {-0.5f,  0.5f,  0.5f} },
		{ { 0.5f,  0.5f,  0.5f} },
		{ { 0.5f, -0.5f,  0.5f} },
	};

	static constexpr uint16_t indices[] =
	{
		// Front
		0, 1, 2,
		0, 2, 3,

		// Back
		4, 6, 5,
		4, 7, 6,

		// Left
		4, 5, 1,
		4, 1, 0,

		// Right
		3, 2, 6,
		3, 6, 7,

		// Top
		1, 5, 6,
		1, 6, 2,

		// Bottom
		4, 0, 3,
		4, 3, 7
	};

	static constexpr UINT vertexBufferSize = sizeof(vertices);
	static constexpr UINT indexBufferSize = sizeof(indices);

	// Define input layout
	static constexpr D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{
			"POSITION",
			0,
			DXGI_FORMAT_R32G32B32_FLOAT,
			0,
			0,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
			0
		}
	};
};