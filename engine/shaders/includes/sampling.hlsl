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