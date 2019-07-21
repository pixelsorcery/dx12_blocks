
StructuredBuffer<float2> coords;

cbuffer modelViewMatrix : register(b0)
{
    float4x4 orthoProj;
};

struct VS_OUTPUT
{
    float4 pos : SV_Position;
    float2 texcoord0 : texcoord0;
};

VS_OUTPUT main(uint id : SV_VertexID)
{
    VS_OUTPUT o;
    uint quadIndex = id / 4;
    uint vertexInQuad = id % 4;
    float4 position;
    switch (vertexInQuad)
    {
        case 0:  position = float4(0, 0, 0, 1); break;
        case 1:  position = float4(1, 0, 0, 1); break;
        case 2:  position = float4(1, 1, 0, 1); break;
        case 3:  position = float4(0, 1, 0, 1); break;
        default: position = float4(0, 0, 0, 0); break;
    }
    //o.pos = mul(position, orthoProj);
    o.pos = position;

    //o.texcoord0 = coords[(quadIndex * 4) + vertexInQuad].xy;
    o.texcoord0 = coords[vertexInQuad];
    return o;
}
