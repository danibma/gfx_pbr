struct VSResult
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
};

Texture2D g_SceneTexture : register(t0);
SamplerState TextureSampler;

float4 main(in VSResult vsResult) : SV_Target
{
    float3 finalColor = g_SceneTexture.Sample(TextureSampler, vsResult.uv).rgb;
    
    return float4(finalColor, 1.0f);

}