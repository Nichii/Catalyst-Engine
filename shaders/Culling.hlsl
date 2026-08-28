// Use the same camera as the graphics pass, otherwise culling and rendering disagree.
cbuffer Camera : register(b0)
{
    float4x4 viewProjection;
};

// Keep this in sync with Renderer::ObjectCount and the command-buffer capacity.
static const uint ObjectCount = 10000;

struct ObjectData
{
    float4x4 worldMatrix;
    float4 bounds;
};

// One thread checks one object and may add one command to the output.
StructuredBuffer<ObjectData> objects : register(t0);
struct IndirectCommand
{
    uint objectID;
    uint indexCount;
    uint instanceCount;
    uint startIndex;
    int baseVertex;
    uint startInstance;
};

// The counter packs visible objects together. Their order does not matter here.
RWStructuredBuffer<IndirectCommand> commands : register(u0);
RWByteAddressBuffer commandCount : register(u1);

[numthreads(64, 1, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    uint objectID = id.x;

    // The last 64-thread group may contain IDs past the end of the object array.
    if (objectID >= ObjectCount)
    {
        return;
    }

    ObjectData object = objects[objectID];

    float3 center = object.bounds.xyz;
    float radius = object.bounds.w;

    float4 clipPosition =
        mul(float4(center, 1.0f), viewProjection);

    // Treat the sphere as visible unless it is clearly outside the view.
    bool visible =
        clipPosition.x >= -clipPosition.w - radius &&
        clipPosition.x <= clipPosition.w + radius &&
        clipPosition.y >= -clipPosition.w - radius &&
        clipPosition.y <= clipPosition.w + radius &&
        clipPosition.z >= 0.0f &&
        clipPosition.z <= clipPosition.w;

    if (visible)
    {
        // Atomically reserve a unique slot; several threads can reach this at once.
        uint commandIndex;
        commandCount.InterlockedAdd(0, 1, commandIndex);
        commands[commandIndex].objectID = objectID;
        commands[commandIndex].indexCount = 36;
        commands[commandIndex].instanceCount = 1;
        commands[commandIndex].startIndex = 0;
        commands[commandIndex].baseVertex = 0;
        commands[commandIndex].startInstance = 0;
    }
}