cbuffer Camera : register(b0)
{
    float4x4 viewProjection;
};

static const uint ObjectCount = 1000;

struct ObjectData
{
    float4x4 worldMatrix;
    float4 bounds;
};

StructuredBuffer<ObjectData> objects : register(t0);
RWStructuredBuffer<uint> visibleObjects : register(u0);

[numthreads(64, 1, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    uint objectID = id.x;

    if (objectID >= ObjectCount)
    {
        return;
    }

    ObjectData object = objects[objectID];

    float3 center = object.bounds.xyz;
    float radius = object.bounds.w;

    float4 clipPosition =
        mul(float4(center, 1.0f), viewProjection);

    bool visible =
        clipPosition.x >= -clipPosition.w - radius &&
        clipPosition.x <= clipPosition.w + radius &&
        clipPosition.y >= -clipPosition.w - radius &&
        clipPosition.y <= clipPosition.w + radius &&
        clipPosition.z >= 0.0f &&
        clipPosition.z <= clipPosition.w;

    visibleObjects[objectID] = visible ? 1 : 0;
}