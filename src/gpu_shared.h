#pragma once

#ifdef __cplusplus

#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/type_aligned.hpp>

typedef uint32_t uint;

typedef glm::ivec2 int2;
typedef glm::ivec4 int4;

typedef glm::uvec2         uint2;
typedef glm::aligned_uvec3 uint3;
typedef glm::uvec4         uint4;

typedef glm::aligned_vec2 float2;
typedef glm::aligned_vec3 float3;
typedef glm::aligned_vec4 float4;

typedef glm::mat4 float4x4;

#define SEMANTIC(X)

#else // HLSL

#define SEMANTIC(X) : X

#endif

struct GPUMaterial
{
	float4 color;
};