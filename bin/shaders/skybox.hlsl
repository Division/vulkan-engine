
struct CameraData
{
    float3 cameraPosition;
    int2 cameraScreenSize;
    float4x4 cameraViewMatrix;
    float4x4 cameraProjectionMatrix;
};

[[vk::binding(0, 0)]]
cbuffer Camera : register(b0) {
    CameraData camera;
};

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

SamplerState SamplerPointWrap;
 
[[vk::binding(11, 0)]]
TextureCube radiance_cubemap : register(t11);

float4 ps_main(VS_out input) : SV_TARGET
{
    float4 skybox_color = radiance_cubemap.Sample(SamplerPointWrap, normalize(input.texcoord));
    //vec4 skybox_color = texture(radiance_cubemap, normalize(texcoord));
	//return float4(input.texcoord, 1.0f);
    return skybox_color;
}