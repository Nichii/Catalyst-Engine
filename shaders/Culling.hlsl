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
    
    if (objectID >= 1000)
        return;
    
    // Temporary
    VisibleObjects[objectID] = 1;
}