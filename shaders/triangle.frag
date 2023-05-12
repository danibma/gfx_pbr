float3 Color;

float4 main() : SV_Target
{
	return float4(Color, 1.0f);
}