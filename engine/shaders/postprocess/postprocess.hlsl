struct PushConstants
{
    float exposure;
};

[[vk::push_constant]] PushConstants push_constants;

struct VS_in
{
    float4 position : POSITION;
    float4 texcoord : TEXCOORD;
};

struct VS_out
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float4 frag_coord : SV_Position;
};

VS_out vs_main(VS_in input)
{
    VS_out result;
    result.texcoord = input.texcoord.xy;
    result.position = float4(input.position.xyz, 1.0f);

    return result;
}

SamplerState SamplerLinearClamp;

Texture2D src_texture : register(t0);

/*
struct Pixel
{
    vec4 value;
};

layout(std140, binding = 1) buffer hdr_data
{
    Pixel image_data[];
};
*/

float4 ToneMap(float4 src_color, float exposure)
{
    //return float4(src_color.rgb / (src_color.rgb + float3(1.0, 1.0, 1.0)), 1.0);
    return float4(float3(1.0, 1.0, 1.0) - exp(-src_color.rgb * exposure), 1.0);
    //return src_color;
}

#define CLUSTER_COUNT_X 4.0f
#define CLUSTER_COUNT_Y 4.0f
#define CLUSTER_COUNT_DEPTH 16.0f
#define CLUSTER_NEAR 0.1f
#define CLUSTER_FAR 50000.0f

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

struct LightGridItem
{
    uint offset;
    uint lightsCount;
    uint projectorsCount;
};

/*float GetSliceIndex(float depth)
{
    float index = LogBase(depth / CLUSTER_NEAR, (float)CLUSTER_FAR / (float)CLUSTER_NEAR) * CLUSTER_COUNT_DEPTH;
    return max(0.0f, index);
}*/

[[vk::binding(7, 0)]] StructuredBuffer<LightGridItem> LightGrid;

float4 ps_main(VS_out input) : SV_TARGET
{
    float4 src_sample = src_texture.Sample(SamplerLinearClamp, input.texcoord);

    float4 light_color = float4(0, 0, 0, 0);
    float2 screenSize = float2(camera.cameraScreenSize);
    float2 pixelCoord = float2(input.frag_coord.x, screenSize.y - input.frag_coord.y);

    uint tileX = uint(floor(pixelCoord.x / screenSize.x * CLUSTER_COUNT_X));
    uint tileY = uint(floor(pixelCoord.y / screenSize.y * CLUSTER_COUNT_Y));
    uint tileSlice = 1;//(uint)GetSliceIndex(input.linear_depth);

    uint pointLightCount = 0;
    for (uint t = 0; t < CLUSTER_COUNT_DEPTH; t++)
    {
        uint cluster_index = tileX + CLUSTER_COUNT_X * tileY + CLUSTER_COUNT_X * CLUSTER_COUNT_Y * t;
        LightGridItem cluster = LightGrid[cluster_index];

        uint lightOffset = cluster.offset;
        pointLightCount += cluster.lightsCount & 0x000fffu;
        uint spotLightCount = cluster.lightsCount >> 16;

        uint projectorCount = cluster.projectorsCount & 0x000fffu;
        uint decalCount = cluster.projectorsCount >> 16;
    }

    /*if (tileX < 3 && tileY < 3)
        light_color.x = v;
    else
        light_color = float4(1, 1, 1, 1);*/
    
    /*
    light_color.r = 1.0f;
    light_color.g = max(1.0f - pointLightCount, 0);
    light_color.b = max(1.0f - pointLightCount, 0);
    

    src_sample *= light_color;
    */

    return ToneMap(src_sample, push_constants.exposure)/* * image_data[0].value*/;
}