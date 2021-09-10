struct PushConstants
{
    float2 uScale;
    float2 uTranslate;
};

[[vk::push_constant]] PushConstants push_constants;

struct VS_in
{
    float4 position : POSITION;
    float4 texcoord0 : TEXCOORD;
    float4 color : COLOR;
};

struct VS_out
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float4 texcoord : TEXCOORD;
};

VS_out vs_main(VS_in input)
{
    VS_out result;
    result.color = input.color;
    result.texcoord = input.texcoord0;
    result.position = float4(input.position.xy * push_constants.uScale + push_constants.uTranslate, 0, 1);

    return result;
}


SamplerState SamplerLinearWrap;
[[vk::binding(0, 0)]] Texture2D font_texture : register(t0);

float4 ps_main(VS_out input) : SV_TARGET
{
    return input.color * font_texture.Sample(SamplerLinearWrap, input.texcoord.xy);
}