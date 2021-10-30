#include "shaders/material/material_default.hlsl"

RWStructuredBuffer<float4> positions : register(u8, space1);

VS_out vs_main(VS_in input, uint instance_id : SV_InstanceID)
{
    VertexData vertex_data = GetDefaultVertexData(input, objectModelMatrix, objectNormalMatrix);

    float3 offset = positions[instance_id].xyz;

    vertex_data.position_worldspace.xyz += offset;

    return GetVSOut(vertex_data, mul(camera.cameraProjectionMatrix, camera.cameraViewMatrix));
}

float4 ps_main(VS_out input) : SV_TARGET
{
    VertexData vertex_data;
    PBRData pbr_data;
    float4 result_color;

    GetPSDataMaterialDefault(input, vertex_data, pbr_data, result_color);

    result_color = float4(abs(vertex_data.position_worldspace.zxy) * 5, 1.0f);

#if defined(LIGHTING)
    float3 light_color = ComputePBRLighting(vertex_data, pbr_data);
    result_color = float4(light_color, result_color.a);
#endif

    return result_color;
}
