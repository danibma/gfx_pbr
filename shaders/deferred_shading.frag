#include "gpu.hlsli"
#include "../src/gpu_shared.h"

GPUMaterial g_Material;

SamplerState LinearWrap;
Texture2D albedo_texture;
Texture2D metallic_texture;
Texture2D roughness_texture;

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
    float4 albedo    = albedo_texture.Sample(LinearWrap, vertexOutput.uv);
    float4 metallic  = metallic_texture.Sample(LinearWrap, vertexOutput.uv);
    float4 roughness = roughness_texture.Sample(LinearWrap, vertexOutput.uv);
    
    DeferredPixelOutput output;
    output.worldPosition = vertexOutput.worldPos;
    output.albedo        = albedo.a == 0 ? g_Material.albedo : albedo;
    output.normal        = float4(vertexOutput.normal, 1.0f);
    output.metallic      = metallic.a == 0 ? g_Material.metallic : metallic;
    output.roughness     = roughness.a == 0 ? g_Material.roughness : roughness;
    
    return output;
}