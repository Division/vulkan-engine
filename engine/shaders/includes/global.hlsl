struct EnvironmentSettingsData
{
    float4 direction_light_color;
    float4 direction_light_direction;
    float exposure;
    float environment_brightness;
};

struct CameraData
{
    float3 cameraPosition;
    float zMin;
    int2 cameraScreenSize;
    float zMax;
    float4x4 cameraViewMatrix;
    float4x4 cameraProjectionMatrix;
};


[[vk::binding(0, 0)]]
cbuffer Camera : register(b0) 
{
    CameraData camera;
};

[[vk::binding(1, 0)]]
cbuffer EnvironmentSettings : register(b1) 
{
    EnvironmentSettingsData environment;
};