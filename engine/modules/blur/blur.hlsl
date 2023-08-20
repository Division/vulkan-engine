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

PS_in VSMain(VS_in input)
{
    PS_in result;
    result.texcoord = input.texcoord.xy;
    result.position = float4(input.position.xyz, 1.0f);

    return result;
}

SamplerState SamplerLinearClamp;

Texture2D src_texture : register(t0);

cbuffer BlurData : register(b1)
{
    float2 blur_direction;
    float4 blur_offsets[40];
    float4 blur_weights[10];
};

float4 PSMainBlur(PS_in input) : SV_TARGET
{
    float2 sample_uv = input.texcoord;

    float4 result = src_texture.Sample(SamplerLinearClamp, sample_uv) * blur_weights[0][0];

    uint radius = uint(max(blur_direction.x, blur_direction.y));
    float2 direction = blur_direction / radius;

    for (uint i = 1; i <= radius; i++)
    {
        float weight = blur_weights[i / 4][i % 4];
        float2 uv_offset = direction * blur_offsets[i].xy;
        result += src_texture.Sample(SamplerLinearClamp, sample_uv + uv_offset) * weight;
        result += src_texture.Sample(SamplerLinearClamp, sample_uv - uv_offset) * weight;
    }

    return result;
}