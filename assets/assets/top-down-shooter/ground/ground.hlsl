#include "shaders/material/material_default.hlsl"

Texture2D albedo1 : register(t8, space1);
Texture2D normal1 : register(t9, space1);
Texture2D albedo2 : register(t10, space1);
Texture2D normal2 : register(t11, space1);
Texture2D albedo3 : register(t12, space1);
Texture2D normal3 : register(t13, space1);
Texture2D splatmap : register(t15, space1);

VS_out vs_main(VS_in input)
{
    VertexData vertex_data = GetDefaultVertexData(input, objectModelMatrix, objectNormalMatrix);
    vertex_data.texcoord0 = vertex_data.position_worldspace.xz * 0.3f;
    return GetVSOut(vertex_data, mul(camera.cameraProjectionMatrix, camera.cameraViewMatrix));
}

float4 ps_main(VS_out input) : SV_TARGET
{
#if defined(DEPTH_ONLY)
    return 1;
#else
    VertexData vertex_data = GePSVertexData(input);

    float4 albedo0_color = texture0.Sample(SamplerLinearWrap, vertex_data.texcoord0);
    float4 albedo1_color = albedo1.Sample(SamplerLinearWrap, vertex_data.texcoord0);
    float4 albedo2_color = albedo2.Sample(SamplerLinearWrap, vertex_data.texcoord0);
    float4 albedo3_color = albedo3.Sample(SamplerLinearWrap, vertex_data.texcoord0);

    float4 normal0_worldspace = GetTBNNormal(vertex_data, normal_map.Sample(SamplerLinearWrap, vertex_data.texcoord0));
    float4 normal1_worldspace = GetTBNNormal(vertex_data, normal1.Sample(SamplerLinearWrap, vertex_data.texcoord0));
    float4 normal2_worldspace = GetTBNNormal(vertex_data, normal2.Sample(SamplerLinearWrap, vertex_data.texcoord0));
    float4 normal3_worldspace = GetTBNNormal(vertex_data, normal3.Sample(SamplerLinearWrap, vertex_data.texcoord0));

    float4 splatmap_values = splatmap.Sample(SamplerLinearWrap, vertex_data.texcoord0 * 0.05f);

    float4 albedo_final = albedo0_color * splatmap_values.x + albedo1_color * splatmap_values.y + albedo2_color * splatmap_values.z + albedo3_color * splatmap_values.w;
    vertex_data.normal_worldspace = normalize(normal0_worldspace * splatmap_values.x + normal1_worldspace * splatmap_values.y + normal2_worldspace * splatmap_values.z + normal3_worldspace * splatmap_values.w);

    PBRData pbr_data;
    pbr_data.albedo = albedo_final.rgb;
    pbr_data.roughness = albedo_final.w;
    pbr_data.metalness = 0.0f;

    float3 light_color = ComputePBRLighting(vertex_data, pbr_data);
    return float4(light_color, 1.0f);
#endif
}