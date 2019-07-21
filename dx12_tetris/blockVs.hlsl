
cbuffer modelViewMatrix : register(b0)
{
	float4x4 orthoProj;
};

struct VS_OUTPUT
{
    float4 pos : SV_Position;
    float2 texcoord0 : texcoord0;
};

struct VS_INPUT
{
    float3 pos : POSITION;
    float2 texcoord0 : TEXCOORD;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT o;
    o.pos = mul(float4(input.pos, 1), orthoProj);
    o.texcoord0 = input.texcoord0;
    return o;
}
