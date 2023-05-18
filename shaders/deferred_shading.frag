#include "gpu.hlsli"
#include "../src/gpu_shared.h"

GPUMaterial g_Material;

SamplerState LinearWrap;
Texture2D albedo_texture;
Texture2D metallic_texture;
Texture2D roughness_texture;
Texture2D emissive_texture;

struct DeferredPixelOutput
{
    float4 worldPosition : SV_Target0;
    float4 albedo : SV_Target1;
    float4 normal : SV_Target2;
    float4 metallic : SV_Target3;
    float4 roughness : SV_Target4;
    float4 emissive : SV_Target5;
};

DeferredPixelOutput main(in DeferredVertexOutput vertexOutput)
{
    float4 albedo    = albedo_texture.Sample(LinearWrap, vertexOutput.uv);
    float4 metallic  = metallic_texture.Sample(LinearWrap, vertexOutput.uv);
    float4 roughness = roughness_texture.Sample(LinearWrap, vertexOutput.uv);
    float4 emissive  = emissive_texture.Sample(LinearWrap, vertexOutput.uv);
    
    float4 final_albedo    = g_Material.albedo;
    float  final_metallic  = g_Material.metallic;
    float  final_roughness = g_Material.roughness;
    
    if (albedo.a != 0)
        final_albedo *= albedo;
    
    if (metallic.a != 0)
        final_metallic *= metallic.r;
    
    if (roughness.a != 0)
        final_roughness *= roughness.r;
    
    DeferredPixelOutput output;
    output.worldPosition = vertexOutput.worldPos;
    output.albedo        = final_albedo;
    output.normal        = float4(vertexOutput.normal, 1.0f);
    output.metallic      = final_metallic;
    output.roughness     = final_roughness;
    output.emissive      = emissive;
    
    return output;
}