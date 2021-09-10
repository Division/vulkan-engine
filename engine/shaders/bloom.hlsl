struct VS_in
{
    float4 position : POSITION;
    float4 texcoord : TEXCOORD;
};

struct PS_in
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

PS_in vs_main(VS_in input)
{
    PS_in result;
    result.texcoord = input.texcoord.xy;
    result.position = float4(input.position.xyz, 1.0f);

    return result;
}

SamplerState SamplerLinearClamp;

Texture2D src_texture : register(t0);
Texture2D src_bloom : register(t1);

cbuffer BloomData : register(b1)
{
    float bloom_threshold;
    float bloom_strength;
};

float4 ps_resample_main(PS_in input) : SV_TARGET
{
    float4 src_sample = src_texture.Sample(SamplerLinearClamp, input.texcoord);

    float brightness = dot(src_sample.rgb, float3(1, 1, 1)) / 3.0f;
    float multiplier = bloom_strength * max(0.0f, brightness - bloom_threshold);
    return src_sample * multiplier;
}

float4 ps_blend_main(PS_in input) : SV_TARGET
{
    float4 src_sample = src_texture.Sample(SamplerLinearClamp, input.texcoord);
    float4 bloom_sample = src_bloom.Sample(SamplerLinearClamp, input.texcoord);
    return src_sample + bloom_sample;
}