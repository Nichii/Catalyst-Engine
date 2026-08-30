#pragma once

#include <DirectXMath.h>
#include <cstdint>

struct Vertex
{
	DirectX::XMFLOAT3 position;
};

struct ObjectData
{
	DirectX::XMFLOAT4X4 worldMatrix;
	DirectX::XMFLOAT4 bounds;
};

struct CameraData
{
	DirectX::XMFLOAT4X4 viewProjection;
};

struct IndirectCommand
{
	uint32_t objectID;
	uint32_t indexCount;
	uint32_t instanceCount;
	uint32_t startIndex;
	int32_t baseVertex;
	uint32_t startInstance;
};

namespace RendererConstants
{
	inline constexpr uint32_t objectCount = 10'000;
	inline constexpr uint32_t commandStride = sizeof(IndirectCommand);
}
