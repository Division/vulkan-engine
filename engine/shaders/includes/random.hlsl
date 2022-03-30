#ifndef _RANDOM_
#define _RANDOM_

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

float get_random_number(uint input)
{
    return hash_normalize(pcg_hash(input));
}

uint random_quantize(float value)
{
    return (uint)(value * 0xffffffffu);
}

#endif