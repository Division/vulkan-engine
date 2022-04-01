#ifndef _RANDOM_
#define _RANDOM_

#include "shaders/includes/math_defines.hlsl"
#include "shaders/includes/quaternion.hlsl"

uint make_random_seed(uint2 p) {
    return 19u * p.x + 47u * p.y + 101u;
}

uint make_random_seed(uint3 p) {
    return 19u * p.x + 47u * p.y + 101u * p.z + 131u;
}

uint make_random_seed(uint4 p) {
    return 19u * p.x + 47u * p.y + 101u * p.z + 131u * p.w + 173u;
}

uint pcg_hash(uint input)
{
    uint state = input * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

uint2 pcg_hash2(uint2 v)
{
    v = v * 1664525u + 1013904223u;

    v.x += v.y * 1664525u;
    v.y += v.x * 1664525u;

    v = v ^ (v >> 16u);

    v.x += v.y * 1664525u;
    v.y += v.x * 1664525u;

    v = v ^ (v >> 16u);

    return v;
}

uint3 pcg_hash3(uint3 v) {

    v = v * 1664525u + 1013904223u;

    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;

    v ^= v >> 16u;

    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;

    return v;
}

float hash_normalize(uint value)
{
    return (float)value * (1.0 / (float)0xffffffffu);
}

float2 hash_normalize2(uint2 value)
{
    return (float2)value * (1.0 / (float2)0xffffffffu);
}

float3 hash_normalize3(uint3 value)
{
    return (float3)value * (1.0 / (float3)0xffffffffu);
}

float get_random_number(uint input)
{
    return hash_normalize(pcg_hash(input));
}

float2 get_random_number2(uint2 input)
{
    return hash_normalize2(pcg_hash2(input));
}

float3 get_random_number3(uint3 input)
{
    return hash_normalize3(pcg_hash3(input));
}

uint random_quantize(float value)
{
    return (uint)(value * 0xffffffffu);
}

uint2 random_quantize2(float2 value)
{
    return (uint2)(value * 0xffffffffu);
}

uint3 random_quantize3(float3 value)
{
    return (uint3)(value * 0xffffffffu);
}

float3 random_point_on_sphere(float3 center, float radius, uint3 seed)
{
    float3 result;

    while (true)
    {
        result = get_random_number3(seed);
        if (result.x * result.x + result.y * result.y + result.z * result.z < 1) break;
        seed = seed * 747796405u + 2891336453u;
    }

    return center + result * radius;
}

float3 random_point_on_line(float3 a, float3 b, uint seed)
{
    return a + (b - a) * get_random_number(seed);
}

float3 random_direction_in_cone(float3 normalized_direction, float cone_angle, uint2 seed)
{
    normalized_direction = float3(0, 1, 0);

    normalized_direction = normalize(normalized_direction);

    float2 random_values = get_random_number2(seed);

    float sigma = random_values.x * 2.0f * PI;
    float alpha = (random_values.y * 2.0f - 1.0f) * cone_angle * 0.5f;

    float3 y_axis = float3(0.0f, 1.0f, 0.0f);

    float3 alpha_rotation_axis = float3(1.0f, 0.0f, 0.0f);
    if (abs(dot(y_axis, normalized_direction)) + 0.0001f < 1.f)
    {
        alpha_rotation_axis = normalize(cross(y_axis, normalized_direction));
    }

    float4 alpha_rotation = quat_rotate_angle_axis(alpha, alpha_rotation_axis);
    float4 sigma_rotation = quat_rotate_angle_axis(sigma, normalized_direction);
    float4 final_rotation = quat_mul(sigma_rotation, alpha_rotation);
    return quat_rotate_vector(normalized_direction, final_rotation);
}


#endif