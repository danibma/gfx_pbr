#include "gpu.hlsli"

float4x4 view;
float4x4 proj;

SkyboxVertexOutput main(float3 pos : Position, float3 normal : Normal, float2 uv : UV)
{
    float4x4 rot_view = view; 
    rot_view._m03_m13_m23 = 0.0; // remove translation
    
    SkyboxVertexOutput output;
    output.local_position = float4(pos, 1.0f);
    output.pixel_position = mul(mul(proj, rot_view), float4(pos, 1.0f));
    output.pixel_position.z = 1.0f;
	
	return output;
}