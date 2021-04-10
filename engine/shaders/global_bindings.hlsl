// The only purpose of this shader is to compile and provide global bindings by reflection
// It's not supposed to render anything

#include "includes/global.hlsl"

struct VS_in
{
    float4 position : POSITION;
};

struct VS_out
{
    float4 position : SV_POSITION;
};

VS_out vs_main(VS_in input)
{
    VS_out result;
    result.position = float4(1, 1, 1, 1);
    return result;
}

[[vk::binding(3,  0)]] Texture2D shadow_map : register(t3);
[[vk::binding(4,  0)]] Texture2D shadow_map_atlas : register(t4);
[[vk::binding(11, 0)]] TextureCube radiance_cubemap : register(t11);
[[vk::binding(12, 0)]] TextureCube irradiance_cubemap : register(t12);
[[vk::binding(13, 0)]] Texture2D brdf_lut : register(t13);

struct LightGridItem
{
    uint offset;
};

struct LightItem
{

    uint mask;
};

struct ProjectorItem
{

    uint mask;
};

[[vk::binding(5, 0)]] cbuffer Lights : register(b5)
{
    LightItem lights[100];
}

[[vk::binding(5, 0)]] cbuffer Projectors : register(b6)
{
    ProjectorItem projectors[100];
};

[[vk::binding(7, 0)]] StructuredBuffer<LightGridItem> LightGrid;

[[vk::binding(8, 0)]] StructuredBuffer<uint> LightIndices;

SamplerState SamplerLinearWrap;

float4 ps_main(VS_out input) : SV_TARGET
{
    float4 result_color = float4(0, 0, 0, 0);

    float4 skybox_color = radiance_cubemap.Sample(SamplerLinearWrap, float3(1,1,1));
    float4 skybox_color2 = irradiance_cubemap.Sample(SamplerLinearWrap, float3(1,1,1));
    float4 brdf = brdf_lut.Sample(SamplerLinearWrap, float2(1,1));
    float4 shadow = shadow_map.Sample(SamplerLinearWrap, float2(1,1)) * shadow_map_atlas.Sample(SamplerLinearWrap, float2(1,1));
    result_color += brdf * skybox_color2 * skybox_color * camera.cameraScreenSize.x * LightIndices[0] * lights[0].mask * environment.exposure * shadow;

    LightGridItem cluster = LightGrid[0];
    result_color.x *= cluster.offset;

    return result_color;
}