#include "gpu.hlsli"

TextureCube g_EnvironmentCube;
SamplerState LinearWrap;

float4 main(SkyboxVertexOutput vertex) : SV_Target
{
    float3 color = g_EnvironmentCube.SampleLevel(LinearWrap, vertex.local_position.xyz, 0).xyz;

    return float4(color, 1.0);
}