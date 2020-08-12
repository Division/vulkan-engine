
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
    float4 color : COLOR;
};

struct VS_out
{
    float4 vertex_color : COLOR;
    float4 position : SV_POSITION;
    [[vk::builtin("PointSize")]] float point_size : PSIZE;
};

VS_out vs_main(VS_in input)
{
    VS_out result;

    result.vertex_color = input.color;
    float4 position_worldspace = input.position;
    result.position = mul(mul(camera.cameraProjectionMatrix, camera.cameraViewMatrix), position_worldspace);
    result.point_size = input.color.w;

    return result;
}

float4 ps_main(VS_out input) : SV_TARGET
{
    return input.vertex_color;
}