
Texture2D<float1> tex : register(t0);
sampler smp : register(s0);

cbuffer colorBuffer : register(b0)
{
    float3 blockColor;
};

struct VS_OUTPUT
{
    float4 pos : SV_Position;
    float2 texcoord0 : texcoord0;
};

float4 main(VS_OUTPUT v) : SV_Target
{
    float4 color = float4(blockColor, 1.0);
    float texColor = tex.Sample(smp, v.texcoord0.xy);
    return color * texColor;
}
