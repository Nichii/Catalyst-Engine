#pragma once

#include <cstdint>

#include <d3d12.h>
#include <d3dcommon.h>
#include <wrl/client.h>

class CullingPass
{
public:
	void Initialize(
		ID3D12Device* device,
		uint32_t objectCount
	);

	void Execute(
		ID3D12GraphicsCommandList* commandList,
		D3D12_GPU_DESCRIPTOR_HANDLE objectSrv,
		D3D12_GPU_DESCRIPTOR_HANDLE visibilityUav,
		D3D12_GPU_VIRTUAL_ADDRESS cameraAddress,
		uint32_t objectCount
	);

	ID3D12Resource* GetVisibilityBuffer() const noexcept;
	uint32_t GetVisibleObjectCount() const noexcept;

private:
	static constexpr uint32_t cullingThreadGroupSize = 64;

	uint32_t m_visibleObjectCount = 0;

	void CreateRootSignature(ID3D12Device* device);
	void CreatePipelineState(ID3D12Device* device);
	void CreateVisibilityBuffer(ID3D12Device* device, uint32_t objectCount);

	Microsoft::WRL::ComPtr<ID3DBlob> m_cullingShader;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_visibilityBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_readbackBuffer;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
};
