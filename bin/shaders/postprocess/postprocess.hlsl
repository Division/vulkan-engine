struct PushConstants
{
    float exposure;
};

[[vk::push_constant]] PushConstants push_constants;

struct VS_in
{
    float4 position : POSITION;
    float4 texcoord : TEXCOORD;
};

struct VS_out
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

VS_out vs_main(VS_in input)
{
    VS_out result;
    result.texcoord = input.texcoord.xy;
    result.position = float4(input.position.xyz, 1.0f);

    return result;
}

SamplerState SamplerLinearClamp;

[[vk::binding(0, 0)]] Texture2D src_texture : register(t0);

/*
struct Pixel
{
    vec4 value;
};

layout(std140, binding = 1) buffer hdr_data
{
    Pixel image_data[];
};
*/

float4 ToneMap(float4 src_color, float exposure)
{
    //return float4(src_color.rgb / (src_color.rgb + float3(1.0, 1.0, 1.0)), 1.0);
    return float4(float3(1.0, 1.0, 1.0) - exp(-src_color.rgb * exposure), 1.0);
    //return src_color;
}

float4 ps_main(VS_out input) : SV_TARGET
{
    float4 src_sample = src_texture.Sample(SamplerLinearClamp, input.texcoord);
    return ToneMap(src_sample, push_constants.exposure)/* * image_data[0].value*/;
}