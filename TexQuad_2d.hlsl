cbuffer constants : register(b0)
{
    float4x4 viewMatrix;
}

struct vertexdata
{
    float2 position : POS;
    float2 texcoord : TEX;
    uint vertexid : SV_VertexID;
};

struct pixeldata
{
    float4 position : SV_POSITION;
    float2 texcoord : TEX;
    float4 color : COL;
};

Texture2D    mytexture : register(t0);
SamplerState mysampler : register(s0);

pixeldata vertex_shader(vertexdata vertex)
{
    pixeldata output;
    output.position = float4(mul((float3x3)viewMatrix, float3(vertex.position, 1.0f)), 1);
    output.texcoord = vertex.texcoord;

    if (vertex.vertexid == 0) output.color = float4(1, 0, 0, 0);
    if (vertex.vertexid == 1) output.color = float4(0, 1, 0, 0);
    if (vertex.vertexid == 2) output.color = float4(0, 0, 1, 0);
    if (vertex.vertexid == 3) output.color = float4(0, 0, 1, 0);
    if (vertex.vertexid == 4) output.color = float4(0, 0, 0, 1);
    if (vertex.vertexid == 5) output.color = float4(1, 0, 0, 0);

    return output;
}

float4 pixel_shader(pixeldata pixel) : SV_TARGET
{
    return mytexture.Sample(mysampler, pixel.texcoord);
    //return float4(pixel.texcoord, 0, 1);
}