#include "gpu.hlsli"

float4x4 view_proj;
float4x4 model;

DeferredVertexOutput main(float3 pos : Position, float3 normal : Normal, float2 uv : UV)
{
    DeferredVertexOutput result;
    
    float4x4 mvp = mul(view_proj, model);
    float4 position = mul(mvp, float4(pos, 1.0f));
    
    result.position = position;
    result.worldPos = mul(model, float4(pos, 1.0f));
    result.normal   = normal;
    
    return result;
}