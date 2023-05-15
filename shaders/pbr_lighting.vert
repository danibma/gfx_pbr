#include "gpu.hlsli"

QuadResult main(uint vertexID : SV_VertexID)
{
	uint vertex = vertexID * 4;
    
	QuadResult result;
	result.position.x = quadVertices[vertex];
	result.position.y = quadVertices[vertex + 1];
	result.position.z = 0.0f;
	result.position.w = 1.0f;
    
	result.uv.x = quadVertices[vertex + 2];
	result.uv.y = quadVertices[vertex + 3];
    
	return result;
}
