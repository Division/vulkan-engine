#include "shaders/material/material_default.hlsl"

VS_out vs_main(VS_in input)
{
    return GetVSMaterialDefault(input);
}

float4 ps_main(VS_out input) : SV_TARGET
{
    return GetPSMaterialDefault(input);
} 