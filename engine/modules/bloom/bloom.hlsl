
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
Texture2D src_texture_low : register(t1);

cbuffer BloomData : register(b2)
{
    float bloom_threshold;
    float2 texel_size;
    float4 low_tex_size; //xy texel size, zw width height
    float Scatter;
    float ClampMax;
    float ThresholdKnee;
};

// copy pasted from: https://github.com/Unity-Technologies/Graphics/blob/master/com.unity.render-pipelines.universal/Shaders/PostProcessing/Bloom.shader

float2 BSpline3Leftmost(float2 x)
{
    return 0.16666667 * x * x * x;
}

float2 BSpline3MiddleLeft(float2 x)
{
    return 0.16666667 + x * (0.5 + x * (0.5 - x * 0.5));
}

float2 BSpline3MiddleRight(float2 x)
{
    return 0.66666667 + x * (-1.0 + 0.5 * x) * x;
}

float2 BSpline3Rightmost(float2 x)
{
    return 0.16666667 + x * (-0.5 + x * (0.5 - x * 0.16666667));
}

void BicubicFilter(float2 fracCoord, out float2 weights[2], out float2 offsets[2])
{
    float2 r = BSpline3Rightmost(fracCoord);
    float2 mr = BSpline3MiddleRight(fracCoord);
    float2 ml = BSpline3MiddleLeft(fracCoord);
    float2 l = 1.0 - mr - ml - r;

    weights[0] = r + mr;
    weights[1] = ml + l;
    offsets[0] = -1.0 + mr * rcp(weights[0]);
    offsets[1] = 1.0 + l * rcp(weights[1]);
}

float4 SampleTexture2DBicubic(Texture2D tex, SamplerState smp, float2 coord, float4 texSize, float2 maxCoord)
{
    float2 xy = coord * texSize.zw + 0.5;
    float2 ic = floor(xy);
    float2 fc = frac(xy);

    float2 weights[2], offsets[2];
    BicubicFilter(fc, weights, offsets);

    return weights[0].y * (weights[0].x * tex.SampleLevel(smp, min((ic + float2(offsets[0].x, offsets[0].y) - 0.5) * texSize.xy, maxCoord), 0.0) +
        weights[1].x * tex.SampleLevel(smp, min((ic + float2(offsets[1].x, offsets[0].y) - 0.5) * texSize.xy, maxCoord), 0.0)) +
        weights[1].y * (weights[0].x * tex.SampleLevel(smp, min((ic + float2(offsets[0].x, offsets[1].y) - 0.5) * texSize.xy, maxCoord), 0.0) +
        weights[1].x * tex.SampleLevel(smp, min((ic + float2(offsets[1].x, offsets[1].y) - 0.5) * texSize.xy, maxCoord), 0.0));
}


float4 ps_prefilter_main(PS_in input) : SV_TARGET
{
    float2 uv = input.texcoord.xy;
    float2 texelSize = texel_size; // why float?

    float4 A = src_texture.Sample(SamplerLinearClamp, uv + texelSize * float2(-1.0, -1.0));
    float4 B = src_texture.Sample(SamplerLinearClamp, uv + texelSize * float2(0.0, -1.0));
    float4 C = src_texture.Sample(SamplerLinearClamp, uv + texelSize * float2(1.0, -1.0));
    float4 D = src_texture.Sample(SamplerLinearClamp, uv + texelSize * float2(-0.5, -0.5));
    float4 E = src_texture.Sample(SamplerLinearClamp, uv + texelSize * float2(0.5, -0.5));
    float4 F = src_texture.Sample(SamplerLinearClamp, uv + texelSize * float2(-1.0, 0.0));
    float4 G = src_texture.Sample(SamplerLinearClamp, uv);
    float4 H = src_texture.Sample(SamplerLinearClamp, uv + texelSize * float2(1.0, 0.0));
    float4 I = src_texture.Sample(SamplerLinearClamp, uv + texelSize * float2(-0.5, 0.5));
    float4 J = src_texture.Sample(SamplerLinearClamp, uv + texelSize * float2(0.5, 0.5));
    float4 K = src_texture.Sample(SamplerLinearClamp, uv + texelSize * float2(-1.0, 1.0));
    float4 L = src_texture.Sample(SamplerLinearClamp, uv + texelSize * float2(0.0, 1.0));
    float4 M = src_texture.Sample(SamplerLinearClamp, uv + texelSize * float2(1.0, 1.0));

    float2 div = (1.0 / 4.0) * float2(0.5, 0.125);

    float4 o = (D + E + I + J) * div.x;
    o += (A + B + G + F) * div.y;
    o += (B + C + H + G) * div.y;
    o += (F + G + L + K) * div.y;
    o += (G + H + M + L) * div.y;

    float3 color = o.xyz;

    // User controlled clamp to limit crazy high broken spec
    color = min(ClampMax, color);

    // Thresholding
    float brightness = max(max(color.r, color.g), color.b);
    float softness = clamp(brightness - bloom_threshold + ThresholdKnee, 0.0, 2.0 * ThresholdKnee);
    softness = (softness * softness) / (4.0 * ThresholdKnee + 1e-4);
    float multiplier = max(brightness - bloom_threshold, softness) / max(brightness, 1e-4);
    color *= multiplier;

    // Clamp colors to positive once in prefilter. Encode can have a sqrt, and sqrt(-x) == NaN. Up/Downsample passes would then spread the NaN.
    color = max(color, 0);
    return float4(color, 1.0f);
}

float4 ps_blur_h_main(PS_in input) : SV_TARGET
{
    float2 uv = input.texcoord.xy;
    float texelSize = texel_size.x * 2;

    // 9-tap gaussian blur on the downsampled source
    float3 c0 = src_texture.Sample(SamplerLinearClamp, uv - float2(texelSize * 4.0, 0.0)).rgb;
    float3 c1 = src_texture.Sample(SamplerLinearClamp, uv - float2(texelSize * 3.0, 0.0)).rgb;
    float3 c2 = src_texture.Sample(SamplerLinearClamp, uv - float2(texelSize * 2.0, 0.0)).rgb;
    float3 c3 = src_texture.Sample(SamplerLinearClamp, uv - float2(texelSize * 1.0, 0.0)).rgb;
    float3 c4 = src_texture.Sample(SamplerLinearClamp, uv).rgb;
    float3 c5 = src_texture.Sample(SamplerLinearClamp, uv + float2(texelSize * 1.0, 0.0)).rgb;
    float3 c6 = src_texture.Sample(SamplerLinearClamp, uv + float2(texelSize * 2.0, 0.0)).rgb;
    float3 c7 = src_texture.Sample(SamplerLinearClamp, uv + float2(texelSize * 3.0, 0.0)).rgb;
    float3 c8 = src_texture.Sample(SamplerLinearClamp, uv + float2(texelSize * 4.0, 0.0)).rgb;

    float3 color = c0 * 0.01621622 + c1 * 0.05405405 + c2 * 0.12162162 + c3 * 0.19459459
                + c4 * 0.22702703
                + c5 * 0.19459459 + c6 * 0.12162162 + c7 * 0.05405405 + c8 * 0.01621622;

    return float4(color, 1.0f);
}

float4 ps_blur_v_main(PS_in input) : SV_TARGET
{
    float2 uv = input.texcoord.xy;
    float texelSize = texel_size.y;

    // Optimized bilinear 5-tap gaussian on the same-sized source (9-tap equivalent)
    float3 c0 = src_texture.Sample(SamplerLinearClamp, uv - float2(0.0, texelSize * 3.23076923)).rgb;
    float3 c1 = src_texture.Sample(SamplerLinearClamp, uv - float2(0.0, texelSize * 1.38461538)).rgb;
    float3 c2 = src_texture.Sample(SamplerLinearClamp, uv).rgb;
    float3 c3 = src_texture.Sample(SamplerLinearClamp, uv + float2(0.0, texelSize * 1.38461538)).rgb;
    float3 c4 = src_texture.Sample(SamplerLinearClamp, uv + float2(0.0, texelSize * 3.23076923)).rgb;

    float3 color = c0 * 0.07027027 + c1 * 0.31621622
        + c2 * 0.22702703
        + c3 * 0.31621622 + c4 * 0.07027027;

    return float4(color, 1.0f);
}


float4 ps_upsample_main(PS_in input) : SV_TARGET
{
    float2 uv = input.texcoord.xy;

    float3 highMip = src_texture.Sample(SamplerLinearClamp, uv).rgb;

    float3 lowMip = SampleTexture2DBicubic(src_texture_low, SamplerLinearClamp, uv, low_tex_size, 1.0);

    //float3 lowMip = src_texture_low.Sample(SamplerLinearClamp, uv).rgb;

    return float4(lerp(highMip, lowMip, Scatter), 1.0f);
}