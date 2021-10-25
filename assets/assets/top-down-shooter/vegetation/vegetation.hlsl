#include "shaders/material/material_default.hlsl"

Texture2D WindTexture : register(t6, space1);

cbuffer UserGlobals : register(b0, space0)
{
    float3 player_position;
};

cbuffer UserMaterial : register(b7, space1)
{
    float wind_multiplier;
};

VS_out vs_main(VS_in input)
{
    VertexData vertex_data = GetDefaultVertexData(input, objectModelMatrix, objectNormalMatrix);

    float3 wind_dir = float3(-1.23f, 0, -1);

    float height_multiplier = vertex_data.position_worldspace.y;

    float2 wind_uv = float2(wind_dir.x, wind_dir.z) * CurrentTime * 0.15f + float2(vertex_data.origin.x, vertex_data.origin.z) * 0.01f;
    float4 wind_value = WindTexture.SampleLevel(SamplerLinearWrap, float2(wind_uv), 0) * 0.5f;
    float3 offset = wind_dir * wind_value.r * height_multiplier * wind_multiplier;

    float3 direction_from_player = vertex_data.origin.xyz - player_position;
    float distance_from_player = length(direction_from_player);
    float player_trigger_distance = 0.7f;

    if (distance_from_player < player_trigger_distance)
    {
        direction_from_player /= (distance_from_player + 1e-4f);

        float lerp_amount = saturate(distance_from_player / player_trigger_distance);
        float offset_amount = saturate((distance_from_player + 0.2f) / player_trigger_distance) * 0.3f * saturate(height_multiplier + 0.2f);
        float3 player_offset = direction_from_player * offset_amount;
        offset = lerp(player_offset, offset, lerp_amount);
    }

    vertex_data.position_worldspace.xyz += offset;

    return GetVSOut(vertex_data, mul(camera.cameraProjectionMatrix, camera.cameraViewMatrix));
}

float4 ps_main(VS_out input) : SV_TARGET
{
    VertexData vertex_data = GePSVertexData(input);
    float4 normal_map_value = normal_map.Sample(SamplerLinearWrap, vertex_data.texcoord0);
    float alpha = normal_map_value.a;
    if (alpha < 0.5f)
        discard;

#if defined(DEPTH_ONLY)
    return 1;
#else
    float4 normal_worldspace = GetTBNNormal(vertex_data, normal_map_value);
    float4 albedo_color = texture0.Sample(SamplerLinearWrap, vertex_data.texcoord0);

    PBRData pbr_data;
    pbr_data.albedo = albedo_color.rgb;
    pbr_data.roughness = 1.0f;
    pbr_data.metalness = 0.0f;

    float3 light_color = ComputePBRLighting(vertex_data, pbr_data);
    return float4(light_color, alpha);
#endif
}