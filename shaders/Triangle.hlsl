cbuffer Camera : register(b0)
{
    float4x4 viewProjection;
};

struct ObjectData
{
    float4x4 modelMatrix;
};

StructuredBuffer<ObjectData> objectDataBuffer : register(t0);

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
    
    ObjectData objData = objectDataBuffer[instanceID];
    
    float4 worldPosition = mul
    (
        float4(input.position, 1.0),
        objData.modelMatrix
    );
    
    output.position = mul(worldPosition, viewProjection);
    
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(0.2f, 0.7f, 1.0f, 1.0f);
}