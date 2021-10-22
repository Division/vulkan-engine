#include "shaders/material/material_default.hlsl"

VS_out vs_main(VS_in input)
{
    return GetVSMaterialDefault(input);
}

float4 ps_main(VS_out input) : SV_TARGET
{
#if defined(DEPTH_ONLY)
    return 1;
#else
    VertexData vertex_data = GePSVertexData(input);

    float4 normal_map_value = normal_map.Sample(SamplerLinearWrap, vertex_data.texcoord0);
    float4 normal_worldspace = GetTBNNormal(vertex_data, normal_map_value);

    float alpha = normal_map_value.a;

    if (alpha < 0.5f)
        discard;

    float4 albedo_color = texture0.Sample(SamplerLinearWrap, vertex_data.texcoord0);

    PBRData pbr_data;
    pbr_data.albedo = albedo_color.rgb;
    pbr_data.roughness = 1.0f;
    pbr_data.metalness = 0.0f;

    float3 light_color = ComputePBRLighting(vertex_data, pbr_data);
    return float4(light_color, alpha);
#endif
}