#include "CullingPass.h"
#include "../Core/Dx12Utils.h"

#include <d3dcompiler.h>
#include <stdexcept>
#include <d3d12.h>
#include <cstdint>
#include <Windows.h>
#include <wrl/client.h>
#include <d3dcommon.h>

void CullingPass::Initialize(
	ID3D12Device* device,
	uint32_t objectCount
)
{
	CreateRootSignature(device);
	CreateVisibilityBuffer(device, objectCount);
	CreatePipelineState(device);
}

ID3D12Resource* CullingPass::GetVisibilityBuffer() const noexcept
{
	return m_visibilityBuffer.Get();
}

uint32_t CullingPass::GetVisibleObjectCount() const noexcept
{
	return m_visibleObjectCount;
}

void CullingPass::Execute(
	ID3D12GraphicsCommandList* commandList,
	D3D12_GPU_DESCRIPTOR_HANDLE objectSrv,
	D3D12_GPU_DESCRIPTOR_HANDLE visibilityUav,
	D3D12_GPU_VIRTUAL_ADDRESS cameraAddress,
	uint32_t objectCount
)
{
	if (!m_visibilityBuffer)
		throw std::runtime_error("Visibility buffer is NULL");

	if (!m_readbackBuffer)
		throw std::runtime_error("Readback buffer is NULL");

	commandList->SetComputeRootSignature(m_rootSignature.Get());

	// b0 - camera
	commandList->SetComputeRootConstantBufferView(0, cameraAddress);

	// t0 - object buffer
	commandList->SetComputeRootDescriptorTable(1, objectSrv);

	// u0 - visibility buffer
	commandList->SetComputeRootDescriptorTable(2, visibilityUav);

	commandList->SetPipelineState(m_pipelineState.Get());

	// Transition the visibility buffer to UAV.
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = m_visibilityBuffer.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	commandList->ResourceBarrier(1, &barrier);

	const UINT dispatchCount =
		(objectCount + cullingThreadGroupSize - 1) / cullingThreadGroupSize;
	commandList->Dispatch(dispatchCount, 1, 1);

	// Synchronize compute writes before copying the visibility buffer.
	barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrier.UAV.pResource = m_visibilityBuffer.Get();

	commandList->ResourceBarrier(1, &barrier);

	// Transition the visibility buffer to copy source.
	barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = m_visibilityBuffer.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	commandList->ResourceBarrier(1, &barrier);

	commandList->CopyResource(m_readbackBuffer.Get(), m_visibilityBuffer.Get());

	// Leave the visibility buffer shader-readable for the graphics pass and next frame.
	barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = m_visibilityBuffer.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	commandList->ResourceBarrier(1, &barrier);
}

void CullingPass::CreateRootSignature(ID3D12Device* device)
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

	hr = device->CreateRootSignature(
		0,
		signatureBlob->GetBufferPointer(),
		signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&m_rootSignature)
	);

	ThrowIfFailed(hr, "Failed to create culling root signature!");
}

void CullingPass::CreatePipelineState(ID3D12Device* device)
{
	// Load and compile Culling.hlsl.
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
	desc.pRootSignature = m_rootSignature.Get();
	desc.CS.pShaderBytecode = m_cullingShader->GetBufferPointer();
	desc.CS.BytecodeLength = m_cullingShader->GetBufferSize();

	hr = device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&m_pipelineState));

	ThrowIfFailed(hr, "Create compute pipeline state failed");
}

void CullingPass::CreateVisibilityBuffer(ID3D12Device* device, uint32_t objectCount)
{
	const UINT64 size = sizeof(uint32_t) * static_cast<UINT64>(objectCount);

	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = size;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	ThrowIfFailed(
		device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_visibilityBuffer)
		),
		"Create visibility buffer"
	);

	D3D12_HEAP_PROPERTIES readbackProperties{};
	readbackProperties.Type = D3D12_HEAP_TYPE_READBACK;

	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ThrowIfFailed(
		device->CreateCommittedResource(
			&readbackProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_readbackBuffer)
		),
		"Create visibility readback buffer"
	);
}
