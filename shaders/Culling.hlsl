cbuffer Camera : register(b0)
{
    float4x4 ViewProjection;
};

struct ObjectData
{
    float4x4 worldMatrix;
    float4 bounds;
};

StructuredBuffer<ObjectData> Objects : register(t0);
RWStructuredBuffer<uint> VisibleObjects : register(u0);

[numthreads(64, 1, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    uint objectID = id.x;
    
    ObjectData object = Objects[objectID];

    float3 center = object.bounds.xyz;
    float radius = object.bounds.w;

    float4 clipPosition =
        mul(float4(center, 1.0f), ViewProjection);

    bool visible =
        clipPosition.x >= -clipPosition.w - radius &&
        clipPosition.x <= clipPosition.w + radius &&
        clipPosition.y >= -clipPosition.w - radius &&
        clipPosition.y <= clipPosition.w + radius &&
        clipPosition.z >= 0.0f &&
        clipPosition.z <= clipPosition.w;

    //VisibleObjects[objectID] = visible ? 1 : 0;
    VisibleObjects[objectID] = objectID < 500 ? 1 : 0;
}