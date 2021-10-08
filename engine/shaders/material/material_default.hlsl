#include "shaders/material/material_pbr.hlsl"

#if defined(TEXTURE0)
Texture2D texture0 : register(t2, space1);
#endif

#if defined(NORMAL_MAP)
Texture2D normal_map : register(t5, space1);
#endif

cbuffer ObjectParams : register(b1, space1)
{
    float4x4 objectModelMatrix;
    float4 color;
    float roughness;
    float metalness;
};

void GetPSDataMaterialDefault(in VS_out input, out VertexData vertex_data, out PBRData pbr_data, out float4 result_color)
{
    vertex_data = GePSVertexData(input);
    result_color = color;

#if defined(TEXTURE0)
    float4 texture0_color = texture0.Sample(SamplerLinearWrap, vertex_data.texcoord0);
    result_color *= texture0_color;
#endif

#if defined(LIGHTING)
    pbr_data.albedo = result_color.rgb;
    pbr_data.roughness = roughness;
    pbr_data.metalness = metalness;

    float3 normal_worldspace_final = normalize(vertex_data.normal_worldspace.xyz);

#if defined (NORMAL_MAP)
    float4 normal_sampled = normal_map.Sample(SamplerLinearWrap, vertex_data.texcoord0);
    vertex_data.normal_worldspace = GetTBNNormal(vertex_data, normal_sampled);

    if (pbr_data.metalness < -0.5f)
    {
        pbr_data.metalness = normal_sampled.w;
    }
#endif

#if defined(TEXTURE0)
    if (pbr_data.roughness < -0.5f)
    {
        pbr_data.roughness = texture0_color.w;
    }
#endif
#else
    pbr_data = GetDefaultPBRData();
#endif
}

VS_out GetVSMaterialDefault(VS_in input)
{
    VertexData vertex_data = GetDefaultVertexData(input, objectModelMatrix);
    return GetVSOut(vertex_data, mul(camera.cameraProjectionMatrix, camera.cameraViewMatrix));
}

float4 GetPSMaterialDefault(VS_out input)
{
    VertexData vertex_data;
    PBRData pbr_data;
    float4 result_color;

    GetPSDataMaterialDefault(input, vertex_data, pbr_data, result_color);

#if defined(LIGHTING)
    float3 light_color = ComputePBRLighting(vertex_data, pbr_data);
    result_color = float4(light_color, result_color.a);
#endif

    return result_color;
}