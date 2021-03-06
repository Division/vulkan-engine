#include "includes/global.hlsl"

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

float SrgbToLinear(float value)
{
    float gamma = 2.2;
    return pow(value, gamma);
}

float LinearizeDepth(float depth_ndc, float near, float far) 
{
    return (2.0 * near * far) / (far + near - (depth_ndc * 2.0f - 1.0f) * (far - near));
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

SamplerState SamplerLinearWrap;
SamplerState SamplerLinearClamp;

float LogBase(float x, float base) 
{
    return log(x) / log(base);
}

#if defined(LIGHTING)
#include "includes/lighting.hlsl"
// TODO: pass textures as params
#include "includes/pbr.hlsl"
#endif

float4 ps_main(VS_out input) : SV_TARGET
{
    float result_alpha = object_params.color.a;
    float4 result_color = object_params.color;
    
#if defined(TEXTURE0)
    float4 texture0_color = texture0.Sample(SamplerLinearWrap, input.texcoord0);
    result_color *= texture0_color;
    result_alpha *= texture0_color.a;
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

    normal_worldspace_final = normalize(mul(TBN, normal_tangentspace));
    if (metalness_final < -0.5f)
    {
        metalness_final = normal_sampled.w;
    }

    #if defined(TEXTURE0)
    if (roughness_final < -0.5f)
    {
        //roughness_final = SrgbToLinear(texture0_color.w);
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
            float attenuation = GetAttenuation(distanceToLight, lights[lightIndex].radius);

            float3 radiance = lights[lightIndex].color.rgb * attenuation;
            float3 lightValue = CalculateLighting(albedo, radiance, normal_worldspace_final, eyeDir_worldspace, -lightDir, roughness_final, metalness_final);
            
            float3 coneDirection = lights[lightIndex].direction;
            float coneAngle = lights[lightIndex].coneAngle;
            float lightToSurfaceAngle = dot(lightDir, coneDirection);
            float innerLightToSurfaceAngle = lightToSurfaceAngle * 1.03;
            float epsilon = innerLightToSurfaceAngle - lightToSurfaceAngle;

            //[branch]
            if (lightToSurfaceAngle > coneAngle && lights[lightIndex].shadowmapScale.x > 0.0) {
                float4 position_lightspace = mul(lights[lightIndex].projectionMatrix, float4(input.position_worldspace.xyz, 1.0));
                float4 position_lightspace_normalized = position_lightspace / position_lightspace.w;
                position_lightspace_normalized.xy = (position_lightspace_normalized.xy + 1.0) / 2.0;
                position_lightspace_normalized.y = 1.0 - position_lightspace_normalized.y;
                float2 shadowmapUV = position_lightspace_normalized.xy * lights[lightIndex].shadowmapScale + lights[lightIndex].shadowmapOffset;
                float shadow = calculateFragmentShadow(shadow_map_atlas, shadowmapUV, position_lightspace_normalized.z);
                lightValue *= 1.0 - shadow;
            }
            
            light_color += float4(lightValue, 0.0);

            //light_color += float4(radiance, 0);
        }
    }

    if (environment.direction_light_enabled) // enabled
    {
        float shadow = 1.0f;

        if (environment.direction_light_cast_shadows)
        {
            float4 position_lightspace = mul(environment.direction_light_projection_matrix, float4(input.position_worldspace.xyz, 1.0));
            float4 position_lightspace_normalized = position_lightspace / position_lightspace.w;
            position_lightspace_normalized.xy = (position_lightspace_normalized.xy + 1.0) / 2.0;
            position_lightspace_normalized.y = 1.0 - position_lightspace_normalized.y;
            float2 shadowmapUV = position_lightspace_normalized.xy;
            shadow = 1.0f - calculateFragmentShadow(shadow_map, shadowmapUV, position_lightspace_normalized.z);
        }

        light_color += shadow * float4(CalculateLighting(albedo, environment.direction_light_color, normal_worldspace_final, eyeDir_worldspace, -environment.direction_light_direction, roughness_final, metalness_final), 0);
    }
    
    float ao = 1.0f;
    float3 ambient = CalculateAmbient(albedo, normal_worldspace_final, eyeDir_worldspace, roughness_final, metalness_final, ao) * environment.environment_brightness;
    result_color = light_color + float4(ambient, 0);
    result_color.a = result_alpha;

#else
    float4 light_color = float4(1.0, 1.0, 1.0, 1.0);
    result_color *= light_color;
#endif

    return result_color;
}