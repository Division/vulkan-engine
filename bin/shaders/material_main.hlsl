
struct CameraData
{
    float3 cameraPosition;
    int2 cameraScreenSize;
    float4x4 cameraViewMatrix;
    float4x4 cameraProjectionMatrix;
};

struct ObjectParamsData 
{
    float4x4 objectModelMatrix;
    float4x4 objectNormalMatrix;
    float2 uvScale;
    float2 uvOffset;
    uint layer;
};

[[vk::binding(0, 0)]]
cbuffer Camera : register(b0) 
{
    CameraData camera;
};

[[vk::binding(1, 1)]]
cbuffer ObjectParams : register(b1) 
{
    ObjectParamsData object_params;
};

struct VS_in
{
    float4 position : POSITION;
#if defined(TEXTURE0)
    float2 texcoord0 : TEXCOORD;
#endif

#if defined(LIGHTING)
    float4 normal : NORMAL;
#endif
};

struct VS_out
{
    float4 position : SV_POSITION;

#if defined(TEXTURE0)
    float2 texcoord0 : TEXCOORD;
#endif

#if defined(LIGHTING)
    float4 normal_worldspace : NORMAL;
#endif

#if !defined(DEPTH_ONLY)
    float4 position_worldspace : POSITION;
#endif

    float4 frag_coord : SV_Position;
};

VS_out vs_main(VS_in input)
{
    VS_out result;
    float4x4 model_matrix = object_params.objectModelMatrix;
#if defined(SKINNING)
    /*model_matrix = mat4(0); // object model transform is included into bone matrices so just blend them
    for (int i = 0; i < 3; i++) {
        int joint_index = int(joint_indices[i]);
        float joint_weight = joint_weights[i];
        model_matrix += skinning.matrices[joint_index] * joint_weight;
    }*/
    not implemented
#endif

    float4 position_worldspace = mul(model_matrix, float4(input.position.xyz, 1.0));
    result.position = mul(mul(camera.cameraProjectionMatrix, camera.cameraViewMatrix), position_worldspace);

#if !defined(DEPTH_ONLY)
    result.position_worldspace = position_worldspace;
#endif

#if defined(LIGHTING)
    result.normal_worldspace = normalize(mul(model_matrix, float4(input.normal.xyz, 0)));
#endif

#if defined(TEXTURE0)
    result.texcoord0 = texcoord0;
#endif

    return result;
}

#define PI 3.1415926535

#if defined(TEXTURE0)
[[vk::binding(2, 1)]] Texture2D texture0 : register(t2);
#endif

#if defined (LIGHTING)
[[vk::binding(3,  0)]] Texture2D shadow_map : register(t3);
[[vk::binding(10, 0)]] TextureCube environment_cubemap : register(t10);
[[vk::binding(11, 0)]] TextureCube radiance_cubemap : register(t11);
[[vk::binding(12, 0)]] TextureCube irradiance_cubemap : register(t12);

struct LightGridItem
{
    uint offset;
    uint lightsCount;
    uint projectorsCount;
};

struct LightItem
{
    float3 position;
    float attenuation;
    float3 color;
    float radius;
    float3 direction;
    float coneAngle;
    float4x4 projectionMatrix;
    float2 shadowmapScale;
    float2 shadowmapOffset;
    uint mask;
};

struct ProjectorItem
{
    float3 position;
    float attenuation;
    float4 color;
    float2 scale;
    float2 offset;
    float2 shadowmapScale;
    float2 shadowmapOffset;
    float4x4 projectionMatrix;
    float radius;
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

#endif

SamplerState SamplerLinearWrap;

float4 ps_main(VS_out input) : SV_TARGET
{
    float4 result_color = float4(1,1,1,1);
    
#if defined(TEXTURE0)
    float4 texture0_color = texture0.Sample(SamplerLinearWrap, normalize(input.texcoord));
    result_color *= texture0_color;
#endif

#if defined(LIGHTING)
    float4 skybox_color = radiance_cubemap.Sample(SamplerLinearWrap, float3(1,1,1));
    result_color += skybox_color * 0.001f;

    float4 light_color = float4(0, 0, 0, 0);
    float TILE_SIZE = 32.0;
    float2 screenSize = float2(camera.cameraScreenSize);
    int2 tilesCount = int2(ceil(screenSize / TILE_SIZE));
    float2 pixelCoord = float2(input.frag_coord.x, screenSize.y - input.frag_coord.y);
    int tileX = int(floor(pixelCoord.x / TILE_SIZE));
    int tileY = int(floor(pixelCoord.y / TILE_SIZE));

    int tileIndex = tileX + tilesCount.x * tileY;
    LightGridItem gridItem = LightGrid[tileIndex];

    uint lightOffset = gridItem.offset;
    uint pointLightCount = gridItem.lightsCount & 0x000fffu;
    uint spotLightCount = gridItem.lightsCount >> 16;

    uint projectorCount = gridItem.projectorsCount & 0x000fffu;
    uint decalCount = gridItem.projectorsCount >> 16;
    float3 eyeDir_worldspace = normalize(camera.cameraPosition - input.position_worldspace.xyz); // vector to camera

    light_color.r = pointLightCount;
    light_color.g = 1.0f - pointLightCount;
    light_color.b = projectorCount;

    result_color *= light_color;

#else
    float4 light_color = float4(1.0, 1.0, 1.0, 1.0);
    result_color *= light_color;
#endif

    return result_color;
}