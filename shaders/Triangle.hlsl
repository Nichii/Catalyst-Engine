// This must match the matrix used by the culling shader.
cbuffer Camera : register(b0)
{
    float4x4 viewProjection;
};

struct ObjectData
{
    float4x4 worldMatrix;
    float4 bounds;
};

// ExecuteIndirect changes this value for each command.
StructuredBuffer<ObjectData> objectDataBuffer : register(t0);
// The command signature writes this root constant for us.
cbuffer DrawCommand : register(b1)
{
    uint objectIndex;
};

struct VSInput
{
    float3 position : POSITION;
};

struct PSInput
{
    float4 position : SV_POSITION;
};

PSInput VSMain(VSInput input, uint instanceID : SV_InstanceID)
{
    PSInput output;
    
    // Look up the transform selected by the current indirect command.
    ObjectData objData = objectDataBuffer[objectIndex];
    
    float4 worldPosition = mul
    (
        float4(input.position, 1.0),
        objData.worldMatrix
    );
    
    output.position = mul(worldPosition, viewProjection);
    
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(0.2f, 0.7f, 1.0f, 1.0f);
}