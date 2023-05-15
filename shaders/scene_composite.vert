struct VSResult
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
};

static const float quadVertices[] =
{
	// positions  // texCoords
    -1.0f,  1.0f,  0.0f,  0.0f,
	-1.0f, -1.0f,  0.0f,  1.0f,
	 1.0f, -1.0f,  1.0f,  1.0f,

	-1.0f,  1.0f,  0.0f,  0.0f,
	 1.0f, -1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,  0.0f
};

VSResult main(uint vertexID : SV_VertexID)
{
    uint vertex = vertexID * 4;
    
    VSResult result;
    result.position.x = quadVertices[vertex];
    result.position.y = quadVertices[vertex + 1];
    result.position.z = 0.0f;
    result.position.w = 1.0f;
    
    result.uv.x = quadVertices[vertex + 2];
    result.uv.y = quadVertices[vertex + 3];
    
    return result;
}
