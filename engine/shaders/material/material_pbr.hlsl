#include "shaders/includes/global.hlsl"
#include "shaders/material/material_common.hlsl"

#if defined(LIGHTING)
#include "shaders/includes/lighting.hlsl"
#include "shaders/includes/pbr.hlsl"
#endif

struct PBRData
{
	float3 albedo;
	float roughness;
	float metalness;
};

PBRData GetDefaultPBRData()
{
    PBRData result;
    result.albedo = 1;
    result.roughness = 1;
    result.metalness = 0;
    return result;
}

float3 ComputePBRLighting(VertexData vertex_data, PBRData pbr_data)
{
#if defined(LIGHTING)

    float4 light_color = float4(0, 0, 0, 0);
    float2 screenSize = float2(camera.cameraScreenSize);
    float2 pixelCoord = float2(vertex_data.frag_coord.x, screenSize.y - vertex_data.frag_coord.y);

    uint tileX = uint(floor(pixelCoord.x / screenSize.x * CLUSTER_COUNT_X));
    uint tileY = uint(floor(pixelCoord.y / screenSize.y * CLUSTER_COUNT_Y));
    uint tileSlice = (uint)GetSliceIndex(vertex_data.linear_depth);

    uint cluster_index = tileX + CLUSTER_COUNT_X * tileY + CLUSTER_COUNT_X * CLUSTER_COUNT_Y * tileSlice;
    LightGridItem cluster = LightGrid[cluster_index];

    uint lightOffset = cluster.offset;
    uint pointLightCount = cluster.lightsCount & 0x000fffu;
    uint spotLightCount = cluster.lightsCount >> 16;

    uint projectorCount = cluster.projectorsCount & 0x000fffu;
    uint decalCount = cluster.projectorsCount >> 16;
    float3 eyeDir_worldspace = normalize(camera.cameraPosition - vertex_data.position_worldspace.xyz); // vector to camera
    
    float3 normal_worldspace_final = normalize(vertex_data.normal_worldspace.xyz);
    float roughness_final = pbr_data.roughness;
    float metalness_final = pbr_data.metalness;
    float3 albedo = pbr_data.albedo;

    uint i;
    //[[loop]]
    for (i = 0; i < pointLightCount; i++) {
        uint currentOffset = lightOffset + i;
        uint lightIndex = LightIndices[currentOffset];

        //[[branch]]
        if (true || (lights[lightIndex].mask) > 0u) {
            float3 lightPosition = lights[lightIndex].position;
            float3 lightDir = vertex_data.position_worldspace.xyz - lightPosition;
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
                float4 position_lightspace = mul(lights[lightIndex].projectionMatrix, float4(vertex_data.position_worldspace.xyz, 1.0));
                float4 position_lightspace_normalized = position_lightspace / position_lightspace.w;
                position_lightspace_normalized.xy = (position_lightspace_normalized.xy + 1.0) / 2.0;
                position_lightspace_normalized.y = 1.0 - position_lightspace_normalized.y;
                float2 shadowmapUV = position_lightspace_normalized.xy * lights[lightIndex].shadowmapScale + lights[lightIndex].shadowmapOffset;
                float shadow = calculateFragmentShadow(shadow_map_atlas, shadowmapUV, position_lightspace_normalized.z);
                lightValue *= 1.0 - shadow;
            }

            light_color += float4(lightValue, 0.0);
        }
    }

    if (environment.direction_light_enabled) // enabled
    {
        float shadow = 1.0f;

        if (environment.direction_light_cast_shadows)
        {
            float4 position_lightspace = mul(environment.direction_light_projection_matrix, float4(vertex_data.position_worldspace.xyz, 1.0));
            float4 position_lightspace_normalized = position_lightspace / position_lightspace.w;
            position_lightspace_normalized.xy = (position_lightspace_normalized.xy + 1.0) / 2.0;
            position_lightspace_normalized.y = 1.0 - position_lightspace_normalized.y;
            float2 shadowmapUV = position_lightspace_normalized.xy;
            shadow = 1.0f - calculateFragmentShadow(shadow_map, shadowmapUV, position_lightspace_normalized.z);
        }

        light_color += shadow * float4(CalculateLighting(albedo, environment.direction_light_color, normal_worldspace_final, eyeDir_worldspace, -environment.direction_light_direction, roughness_final, metalness_final), 0);
    }

    float3 ambient = CalculateAmbient(albedo, normal_worldspace_final, eyeDir_worldspace, roughness_final, metalness_final) * environment.environment_brightness;
    return light_color.rgb  + ambient;
    
#else
    return 1;
#endif

}