#include "shaders/material/material_default.hlsl"

VS_out vs_main(VS_in input)
{
    VertexData vertex_data = GetDefaultVertexData(input, objectModelMatrix);
    vertex_data.texcoord0 = vertex_data.position_worldspace.xz * 0.1f;
    return GetVSOut(vertex_data, mul(camera.cameraProjectionMatrix, camera.cameraViewMatrix));
}

float4 ps_main(VS_out input) : SV_TARGET
{
    return GetPSMaterialDefault(input);
}