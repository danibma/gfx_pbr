#pragma once

static const float PI = 3.14159265359;

struct DeferredVertexOutput
{
	float4 position : SV_Position;
    float4 worldPos : WorldPosition;
	float3 normal	: OutNormal;
};

struct QuadResult
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

static const float quadVertices[] =
{
	// positions  // texCoords
    -1.0f, 1.0f, 0.0f, 0.0f,
	-1.0f, -1.0f, 0.0f, 1.0f,
	 1.0f, -1.0f, 1.0f, 1.0f,

	-1.0f, 1.0f, 0.0f, 0.0f,
	 1.0f, -1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 0.0f
};