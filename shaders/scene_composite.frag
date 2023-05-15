#include "gpu.hlsli"

Texture2D g_SceneTexture : register(t0);
SamplerState TextureSampler;

float3 ReinhardTonemap(float3 finalColor)
{
    return finalColor / (finalColor + (float3) 1.0f);
}

float4 main(in QuadResult quad) : SV_Target
{
    float3 finalColor = g_SceneTexture.Sample(TextureSampler, quad.uv).rgb;
    
    finalColor = ReinhardTonemap(finalColor); // tonemapping
    finalColor = pow(finalColor, (float3)(1.0 / 2.2)); // gamma correction
    
    return float4(finalColor, 1.0f);

}