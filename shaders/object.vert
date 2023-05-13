float4x4 view_proj;
float4x4 model;

float4 main(float3 pos : Position, float3 normal : Normal, float2 uv : UV) : SV_Position
{
    float4x4 mvp = mul(view_proj, model);
    float4 position = mul(mvp, float4(pos, 1.0f));
    
    return position;
}