#include "gpu.hlsli"

Texture2D g_SceneTexture : register(t0);
SamplerState TextureSampler;

float3 AcesFilm(const float3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

float3 ReinhardTonemap(float3 finalColor)
{
    return finalColor / (finalColor + (float3) 1.0f);
}

float4 main(in QuadResult quad) : SV_Target
{
    float3 finalColor = g_SceneTexture.Sample(TextureSampler, quad.uv).rgb;
    
    finalColor = AcesFilm(finalColor); // tonemapping
    finalColor = pow(finalColor, (float3)(1.0 / 2.2)); // gamma correction
    
    return float4(finalColor, 1.0f);

}