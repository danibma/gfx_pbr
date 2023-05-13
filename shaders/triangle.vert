float4x4 view_proj;
float4x4 model;

float4 main(float3 pos : Position) : SV_Position
{
    float4 position = mul(view_proj, float4(pos, 1.0f));
    
    return position;
}