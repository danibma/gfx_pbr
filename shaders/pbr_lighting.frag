#include "gpu.hlsli"

float DistributionGGX(float3 N, float3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0f);
	float NdotH2 = NdotH * NdotH;

	float num = a2;
	float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
	denom = PI * denom * denom;
    
	return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = roughness + 1.0f;
	float k = (r * r) / 8.0f;
    
	float num   = NdotV;
	float denom = NdotV * (1.0f - k) + k;
    
	return num / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0f);
	float NdotL = max(dot(N, L), 0.0f);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
	return ggx1 * ggx2;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
	return F0 + (1.0f - F0) * pow(clamp(1.0 - cosTheta, 0.0f, 1.0f), 5.0f);
}

// Camera info
float4 camPos;

// Light info
float3 lightPos;
float3 lightColor;

SamplerState	  g_TextureSampler;
Texture2D<float4> g_WorldPosition;
Texture2D<float4> g_Albedo;
Texture2D<float4> g_Normal;
Texture2D<float4> g_Metallic;
Texture2D<float4> g_Roughness;

float4 main(QuadResult quad) : SV_Target
{
	// gbuffer values
	float3 worldPos  = g_WorldPosition.Sample(g_TextureSampler, quad.uv).rgb;
	float3 albedo    = g_Albedo.Sample(g_TextureSampler, quad.uv).rgb;
	float3 normal    = g_Normal.Sample(g_TextureSampler, quad.uv).rgb;
	float  metallic  = g_Metallic.Sample(g_TextureSampler, quad.uv).r;
	float  roughness = g_Roughness.Sample(g_TextureSampler, quad.uv).r;
	
	float4 finalColor;

	float3 N = normalize(normal);
	float3 V = normalize(camPos.rgb - worldPos); // view direction
    
	float3 F0 = (float3)0.04f;
	F0 = lerp(F0, albedo, metallic);
    
    // reflectance equation
	float3 Lo = (float3)0.0f;
    
    // do this per light, atm we only have one
	float3 L = normalize(lightPos - worldPos); // light direction
	float3 H = normalize(V + L); // half vector
	float distance = length(lightColor - worldPos);
	float attenuation = 1.0f / (distance * distance);
	float3 radiance = lightColor * attenuation;
    
    // cook-torrance brdf
	float NDF = DistributionGGX(N, H, roughness);
	float G   = GeometrySmith(N, V, L, roughness);
	float3 F  = FresnelSchlick(max(dot(H, V), 0.0f), F0);
    
	float3 kS = F;
	float3 kD = (float3) 1.0f - kS;
	kD *= 1.0f - metallic;

	// Lambertian diffuse
    float3 diffuse = ((kD * albedo) / PI);
    
	// Microfacet Specular BRDF
	float3 numerator = NDF * G * F;
	float denominator = 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0) + 0.0001;
	float3 specular = numerator / denominator;
    
    // add to outgoing radiance Lo
	float NdotL = max(dot(N, L), 0.0f);
    Lo += (diffuse + specular) * radiance * NdotL;
    
	float3 ambient = (float3)0.03f * albedo;
	float3 color = ambient * Lo;
    
	finalColor = float4(color, 1.0f);
    
	return finalColor;
}