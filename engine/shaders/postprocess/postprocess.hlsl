struct VS_in
{
    float4 position : POSITION;
    float4 texcoord : TEXCOORD;
};

struct VS_out
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float4 frag_coord : SV_Position;
};

VS_out vs_main(VS_in input)
{
    VS_out result;
    result.texcoord = input.texcoord.xy;
    result.position = float4(input.position.xyz * 0.3f, 1.0f);

    return result;
}

cbuffer BloomData : register(b2)
{
    float bloom_threshold;
    float bloom_strength;
    float2 texel_size;
    float4 low_tex_size; //xy texel size, zw width height
    float Scatter;
    float ClampMax;
    float ThresholdKnee;
};

SamplerState SamplerLinearClamp;

float4 ps_main(VS_out input) : SV_TARGET
{
    return float4(0, 1, 0, bloom_threshold);
}