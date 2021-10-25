struct EnvironmentSettingsData
{
    float4x4 direction_light_projection_matrix;
    float3 direction_light_color;
    uint direction_light_enabled;
    float3 direction_light_direction;
    uint direction_light_cast_shadows;
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


cbuffer GlobalConstants : register(b1, space0)
{
    EnvironmentSettingsData environment;
    CameraData camera;
    float CurrentTime;
};