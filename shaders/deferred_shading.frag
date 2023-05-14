#include "gpu.hlsli"
#include "../src/gpu_shared.h"

GPUMaterial g_Material;

struct DeferredPixelOutput
{
    float4 color : SV_Target0;
    float4 normal : SV_Target1;
};

DeferredPixelOutput main(in DeferredVertexOutput vertexOutput)
{
    DeferredPixelOutput output;
    output.color  = g_Material.color;
    output.normal = float4(vertexOutput.normal, 1.0f);
    
    return output;
}