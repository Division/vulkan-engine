
#include "shaders/includes/global.hlsl"

struct VS_in
{
    float4 position : POSITION;
};

struct VS_out
{
    float4 position : SV_POSITION;
    float3 texcoord : TEXCOORD;
};

VS_out vs_main(VS_in input)
{
    VS_out result;
    result.texcoord = input.position.xyz;
    float3 position_cameraspace = mul(camera.cameraViewMatrix, float4(input.position.xyz, 0)).xyz;
    result.position = normalize(mul(camera.cameraProjectionMatrix, float4(position_cameraspace, 1.0f)));

    return result;
}

SamplerState SamplerLinearWrap;
 
TextureCube radiance_cubemap : register(t11);

float4 ps_main(VS_out input) : SV_TARGET
{
    float4 skybox_color = radiance_cubemap.Sample(SamplerLinearWrap, normalize(input.texcoord));
    return skybox_color * environment.environment_brightness;
}