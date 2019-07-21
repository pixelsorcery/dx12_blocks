
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
	if (v.texcoord0.x < 0.05 || v.texcoord0.x > 0.95 || 
	    v.texcoord0.y < 0.05 || v.texcoord0.y > 0.95)
	{
		return float4(0.0, 0.0, 0.0, 1.0);
	}

    return float4(blockColor, 1.0);
}
