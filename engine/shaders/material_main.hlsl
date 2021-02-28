
struct CameraData
{
    float3 cameraPosition;
    float zMin;
    int2 cameraScreenSize;
    float zMax;
    float4x4 cameraViewMatrix;
    float4x4 cameraProjectionMatrix;
};

struct ObjectParamsData 
{
    float4x4 objectModelMatrix;
    float4x4 objectNormalMatrix;
    float4 color;
    float2 uvScale;
    float2 uvOffset;
    uint layer;
    float roughness;
    float metalness;
};

struct SkinningMatricesData
{
    float4x4 matrices[70];
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

#if defined(SKINNING)
[[vk::binding(3, 1)]]
cbuffer SkinningMatrices : register(b3) 
{
    SkinningMatricesData skinning_matrices;
};

#endif

struct VS_in
{
    float4 position : POSITION;
#if defined(TEXTURE0)
    float2 texcoord0 : TEXCOORD;
#endif

#if defined(LIGHTING)
    float4 normal : NORMAL;
#if defined(NORMAL_MAP)
    float4 tangent : TANGENT;
#endif
#endif

#if defined(SKINNING)
    float4 joint_weights : BLENDWEIGHT;
    uint4 joint_indices : BLENDINDICES;
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
    float4 tangent_worldspace : TANGENT;
    float linear_depth : POSITION1;
#endif

#if !defined(DEPTH_ONLY)
    float4 position_worldspace : POSITION;
#endif

    float4 frag_coord : SV_Position;
};

float LinearizeDepth(float depth_ndc, float near, float far) 
{
    return (2.0 * near * far) / (far + near - depth_ndc * (far - near));
}

VS_out vs_main(VS_in input)
{
    VS_out result;
    float4x4 model_matrix = object_params.objectModelMatrix;
#if defined(SKINNING)
    model_matrix = float4x4(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);
    [unroll]
    for (int i = 0; i < 4; i++)
    {
        uint joint_index = input.joint_indices[i];
        float joint_weight = input.joint_weights[i];
        model_matrix += skinning_matrices.matrices[joint_index] * joint_weight;
    }
#endif

    float4 position_worldspace = mul(model_matrix, float4(input.position.xyz, 1.0));
    result.position = mul(mul(camera.cameraProjectionMatrix, camera.cameraViewMatrix), position_worldspace);

#if !defined(DEPTH_ONLY)
    result.position_worldspace = position_worldspace;
#endif

#if defined(LIGHTING)
    result.normal_worldspace = normalize(mul(model_matrix, float4(input.normal.xyz, 0)));
    result.tangent_worldspace = 0.0f;
    #if defined(NORMAL_MAP)
    result.tangent_worldspace = normalize(mul(model_matrix, float4(input.tangent.xyz, 0)));
    result.tangent_worldspace.w = input.tangent.w;
    #endif
    result.linear_depth = LinearizeDepth(result.position.z / result.position.w, camera.zMin, camera.zMax);
#endif

#if defined(TEXTURE0)
    result.texcoord0 = input.texcoord0;
#endif

    return result;
}

#define PI 3.1415926535

#if defined(TEXTURE0)
[[vk::binding(2, 1)]] Texture2D texture0 : register(t2);
#endif

#if defined(NORMAL_MAP)
[[vk::binding(4, 1)]] Texture2D normal_map : register(t4);
#endif


#if defined (LIGHTING)
[[vk::binding(3,  0)]] Texture2D shadow_map : register(t3);
/*[[vk::binding(10, 0)]] TextureCube environment_cubemap : register(t10);
[[vk::binding(11, 0)]] TextureCube radiance_cubemap : register(t11);
[[vk::binding(12, 0)]] TextureCube irradiance_cubemap : register(t12);
*/
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

float GetAttenuation(float distance, float radius)
{
    float lightInnerR = radius * 0.01;
    float d = max(distance, lightInnerR);
    return clamp(1.0 - pow(d / radius, 4.0), 0.0, 1.0) / (d * d + 1.0);
}

#endif

SamplerState SamplerLinearWrap;

#include "includes/pbr_t.inc"

float LogBase(float x, float base) 
{
    return log(x) / log(base);
}

#define CLUSTER_COUNT_X 4.0f
#define CLUSTER_COUNT_Y 4.0f
#define CLUSTER_COUNT_DEPTH 16.0f
#define CLUSTER_NEAR 0.1f
#define CLUSTER_FAR 50000.0f

float GetSliceIndex(float depth)
{
    float index = LogBase(depth / CLUSTER_NEAR, (float)CLUSTER_FAR / (float)CLUSTER_NEAR) * CLUSTER_COUNT_DEPTH;
    return max(0.0f, index);
}

float4 ps_main(VS_out input) : SV_TARGET
{
    float4 result_color = object_params.color;
    
#if defined(TEXTURE0)
    float4 texture0_color = texture0.Sample(SamplerLinearWrap, input.texcoord0);
    result_color *= texture0_color;
#endif
 
#if defined(LIGHTING)
    float roughness_final = object_params.roughness;
    float metalness_final = object_params.metalness;
    float3 normal_worldspace_final = normalize(input.normal_worldspace.xyz);
    
    #if defined (NORMAL_MAP)
    float4 normal_sampled = normal_map.Sample(SamplerLinearWrap, input.texcoord0);
    float3 normal_tangentspace = normalize(normal_sampled.xyz * 2.0f - 1.0f);

    float3 tangent_worldspace_final = normalize(input.tangent_worldspace.xyz);
    float3 bitangent_worldspace_final = normalize(cross(normal_worldspace_final, tangent_worldspace_final) * input.tangent_worldspace.w);

    float3x3 TBN = transpose(float3x3(tangent_worldspace_final, bitangent_worldspace_final, normal_worldspace_final));
    //float3x3 TBN = transpose(float3x3(bitangent_worldspace_final, tangent_worldspace_final, normal_worldspace_final));

    normal_worldspace_final = normalize(mul(TBN, normal_tangentspace));
    if (metalness_final < 0.0f)
    {
        metalness_final = normal_sampled.w;
    }

    #if defined(TEXTURE0)
    if (roughness_final < 0.0f)
    {
        roughness_final = texture0_color.w;
    }
    #endif
    #endif


    float4 light_color = float4(0, 0, 0, 0);
    float2 screenSize = float2(camera.cameraScreenSize);
    float2 pixelCoord = float2(input.frag_coord.x, screenSize.y - input.frag_coord.y);

    uint tileX = uint(floor(pixelCoord.x / screenSize.x * CLUSTER_COUNT_X));
    uint tileY = uint(floor(pixelCoord.y / screenSize.y * CLUSTER_COUNT_Y));
    uint tileSlice = (uint)GetSliceIndex(input.linear_depth);

    uint cluster_index = tileX + CLUSTER_COUNT_X * tileY + CLUSTER_COUNT_X * CLUSTER_COUNT_Y * tileSlice;
    LightGridItem cluster = LightGrid[cluster_index];

    uint lightOffset = cluster.offset;
    uint pointLightCount = cluster.lightsCount & 0x000fffu;
    uint spotLightCount = cluster.lightsCount >> 16;

    uint projectorCount = cluster.projectorsCount & 0x000fffu;
    uint decalCount = cluster.projectorsCount >> 16;
    float3 eyeDir_worldspace = normalize(camera.cameraPosition - input.position_worldspace.xyz); // vector to camera
    float3 albedo = result_color.xyz;
    float ao = 0.0f;

    uint i;
    //[[loop]]
    for (i = 0; i < pointLightCount; i++) {
        uint currentOffset = lightOffset + i;
        uint lightIndex = LightIndices[currentOffset];

        //[[branch]]
        if (true || (lights[lightIndex].mask/* & layer*/) > 0u) {
            float3 lightPosition = lights[lightIndex].position;
            float3 lightDir = input.position_worldspace.xyz - lightPosition;
            float distanceToLight = length(lightDir);
            lightDir /= distanceToLight; // normalize
            float attenuation = 1.0f;// GetAttenuation(distanceToLight, lights[lightIndex].radius);
            //lightDir = float3(1, 0, 0);

            //vec3 lightValue = calculateFragmentDiffuse(normalizedDistanceToLight, lights[lightIndex].attenuation, normal_worldspace.xyz, lightDir, eyeDir_worldspace, lights[lightIndex].color, materialSpecular);
            float3 radiance = lights[lightIndex].color.rgb * attenuation;
            float3 lightValue = CalculateLighting(albedo, radiance, normal_worldspace_final, eyeDir_worldspace, -lightDir, roughness_final, metalness_final);
            //float3 lightValue = CalculateLighting(albedo, radiance, normalize(input.normal_worldspace.xyz), eyeDir_worldspace, -lightDir, 0.9f, 0.0f);

            /*
            float3 coneDirection = lights[lightIndex].direction;
            float coneAngle = lights[lightIndex].coneAngle;
            float lightToSurfaceAngle = dot(lightDir, coneDirection);
            float innerLightToSurfaceAngle = lightToSurfaceAngle * 1.03;
            float epsilon = innerLightToSurfaceAngle - lightToSurfaceAngle;

            //[branch]
            if (lightToSurfaceAngle > coneAngle && lights[lightIndex].shadowmapScale.x > 0.0) {
                vec4 position_lightspace = lights[lightIndex].projectionMatrix * vec4(position_worldspace.xyz, 1.0);

                vec4 position_lightspace_normalized = position_lightspace / position_lightspace.w;
                position_lightspace_normalized.xy = (position_lightspace_normalized.xy + 1.0) / 2.0;
                position_lightspace_normalized.y = 1.0 - position_lightspace_normalized.y;
                vec2 shadowmapUV = position_lightspace_normalized.xy * lights[lightIndex].shadowmapScale + lights[lightIndex].shadowmapOffset;
                float shadow = calculateFragmentShadow(shadowmapUV, position_lightspace_normalized.z);
                lightValue *= 1.0 - shadow;
            }
            */
            light_color += float4(lightValue, 0.0);

            //light_color += float4(radiance, 0);
        }
    }

    float3 ambient = float3(0.03, 0.03, 0.03) * albedo * ao;
    light_color.a = 1.0f;
    result_color = result_color * light_color + float4(ambient, 0);

    //result_color = result_color * 0.00001 + float4(0.5,0.5,0.5, 1);
    //result_color = pow((result_color + 0.055) / 1.055, 2.4); 

    // result_color.rgb *= 0.00001;
    // result_color.rgb += input.normal_worldspace.xyz;

#else
    float4 light_color = float4(1.0, 1.0, 1.0, 1.0);
    result_color *= light_color;
#endif

    return result_color;
}