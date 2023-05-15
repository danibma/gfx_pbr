#include "gpu.hlsli"
#include "../src/gpu_shared.h"

GPUMaterial g_Material;

struct DeferredPixelOutput
{
    float4 worldPosition : SV_Target0;
    float4 albedo : SV_Target1;
    float4 normal : SV_Target2;
    float4 metallic : SV_Target3;
    float4 roughness : SV_Target4;
};

DeferredPixelOutput main(in DeferredVertexOutput vertexOutput)
{
    DeferredPixelOutput output;
    output.worldPosition = vertexOutput.worldPos;
    output.albedo        = g_Material.albedo;
    output.normal        = float4(vertexOutput.normal, 1.0f);
    output.metallic      = (float4)g_Material.metallic;
    output.roughness     = (float4)g_Material.roughness;
    
    return output;
}