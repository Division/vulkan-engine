#pragma once

#include "murmurhash/MurmurHash3.h"
#include <math.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <half/half.h>
#include <ctime>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define PI M_PI
#define RAD(value) (value * (float)PI / 180)
#define DEG(value) (value / (float)PI * 180)
#define M_TO_STRING(value) to_string(value)

using namespace glm;

extern const vec3 VectorLeft;
extern const vec3 VectorRight;
extern const vec3 VectorUp;
extern const vec3 VectorDown;
extern const vec3 VectorForward;
extern const vec3 VectorBack;

inline quat QuatLookAt(const vec3& direction, const vec3& up = vec3(0, 1, 0))
{
    mat3 Result;

    Result[2] = -direction;
    vec3 Right = glm::normalize(cross(up, Result[2]));
    Result[0] = glm::normalize(Right * inversesqrt(glm::max(0.00001f, dot(Right, Right))));
    Result[1] = glm::normalize(cross(Result[2], Result[0]));

    return quat_cast(Result);
}


namespace glm {
	float dotf(const vec2 &a, const vec3 &b);
	float dotf(const vec3 &a, const vec3 &b);
	float dotf(const vec4 &a, const vec4 &b);
}

struct Sphere {
  vec3 position;
  float radius;
};

struct OBB {
  mat4 matrix;
  vec3 min = vec3(-1);
  vec3 max = vec3(1);

  OBB() = default;
  OBB(const mat4 &matrix, const vec3 &min, const vec3 &max) {
    this->min = min;
    this->max = max;
    this->matrix = matrix;
  }
};

// TODO: may be not portable
struct Vector4_A2R10G10B10
{
    int b : 10;
    int g : 10;
    int r : 10;
    int a : 2;

    static constexpr uint32_t MaxRange(uint32_t bitsize)
    {
        return (1 << (bitsize - 1)) - 1;
    }

    Vector4_A2R10G10B10() : a(0), r(0), g(0), b(0) {};

    Vector4_A2R10G10B10(int a, int r, int g, int b) : a(a), r(r), g(g), b(b) {}

    static Vector4_A2R10G10B10 FromSignedNormalizedFloat(vec4 src)
    {
        src = glm::max(-glm::one<vec4>(), glm::min(src, glm::one<vec4>()));
        return Vector4_A2R10G10B10(
            src.w * MaxRange(2),
            src.x * MaxRange(10),
            src.y * MaxRange(10),
            src.z * MaxRange(10)
        );
    }

    operator vec4() const
    {
        return glm::max(-glm::one<vec4>(), vec4(r / (float)MaxRange(10), g / (float)MaxRange(10), b / (float)MaxRange(10), a / (float)MaxRange(2)));
    }
};

struct Vector2Half
{
    FLOAT16 x, y;

    Vector2Half() = default;
    Vector2Half(float x, float y) : x(x), y(y) {};

    operator vec2() const
    {
        return vec2(FLOAT16::ToFloat32(x), FLOAT16::ToFloat32(y));
    }
};

struct Vector4b
{
    union
    {
        uint8_t data[4];
        struct
        {
            uint8_t x, y, z, w;
        };
    };

    Vector4b() = default;
    Vector4b(uint8_t x, uint8_t y, uint8_t z, uint8_t w) : x(x), y(y), z(z), w(w) {};
    static Vector4b FromNormalizedFloat(vec4 src) 
    {
        return Vector4b(
            (uint8_t)(std::min(1.0f, std::max(src.x, 0.0f)) * std::numeric_limits<uint8_t>::max()),
            (uint8_t)(std::min(1.0f, std::max(src.y, 0.0f)) * std::numeric_limits<uint8_t>::max()),
            (uint8_t)(std::min(1.0f, std::max(src.z, 0.0f)) * std::numeric_limits<uint8_t>::max()),
            (uint8_t)(std::min(1.0f, std::max(src.w, 0.0f)) * std::numeric_limits<uint8_t>::max())
        );
    }

    vec4 ToNormalizedFloat() const
    {
        return vec4(x, y, z, w) / (float)std::numeric_limits<uint8_t>::max();
    }

    static Vector4b FromUInt(ivec4 src)
    {
        return Vector4b(
            (uint8_t)src.x, (uint8_t)src.y, (uint8_t)src.z, (uint8_t)src.w
        );
    }

};

struct AABB 
{
    vec3 min;
    vec3 max;

    AABB() = default;

    AABB(const AABB& other) : min(other.min), max(other.max) {}

    AABB(const vec3 &vmin, const vec3 &vmax) 
    {
        min = vmin;
        max = vmax;
    }

    vec3 size() { return max - min; }

    void expand(const vec3 &point) 
    {
        min = glm::min(point, min);
        max = glm::max(point, max);
    }

    static AABB fromSphere(const vec3 &position, float radius) 
    {
        vec3 radiusVec = vec3(radius, radius, radius);
        return AABB(position - radiusVec, position + radiusVec);
    }

    bool intersectsAABB(const AABB &other) 
    {

        return (min.x <= other.max.x && max.x >= other.min.x) &&
              (min.y <= other.max.y && max.y >= other.min.y) &&
              (min.z <= other.max.z && max.z >= other.min.z);
    }

    AABB project(const mat4 &modelMatrix, const mat4 &projectionMatrix, const vec4 &viewport) 
    {
        vec3 s = size();

        const vec3 vertices[] = {
            glm::project(min, modelMatrix, projectionMatrix, viewport),
            glm::project(min + s * vec3(0, 0, 1), modelMatrix, projectionMatrix, viewport),
            glm::project(min + s * vec3(1, 0, 1), modelMatrix, projectionMatrix, viewport),
            glm::project(min + s * vec3(1, 0, 0), modelMatrix, projectionMatrix, viewport),
            glm::project(max - s * vec3(1, 0, 1), modelMatrix, projectionMatrix, viewport),
            glm::project(max - s * vec3(1, 0, 0), modelMatrix, projectionMatrix, viewport),
            glm::project(max, modelMatrix, projectionMatrix, viewport),
            glm::project(max - s * vec3(0, 0, 1), modelMatrix, projectionMatrix, viewport),
        };

        AABB result (vertices[0], vertices[0]);
        for (int i = 1; i < 8; i++) {
            result.expand(vertices[i]);
        }

        return result;
    }
};

Sphere boundingSphereForFrustum(float width, float height, float zNear, float zFar, float fov);

struct Rect {
  float x = 0;
  float y = 0;
  float width = 0;
  float height = 0;

  Rect() = default;
  Rect(float x, float y, float width, float height) : x(x), y(y), width(width), height(height) {}
  operator vec4() const { return vec4(x, y, width, height); }
};

struct Plane
{
    vec3 center;
    vec3 normal;

    bool IntersectRay(vec3 ray_origin, vec3 ray_direction, vec3* out_position)
    {
        vec3 diff = ray_origin - center;
        float prod1 = glm::dot(diff, normal);
        float prod2 = glm::dot(ray_direction, normal);

        if (std::abs(prod2) < 1e-6f)
            return false;

        float prod3 = prod1 / prod2;
        if (prod3 > 0)
            return false;

        if (out_position)
            *out_position = ray_origin - ray_direction * prod3;

        return true;
    }
};

inline int32_t getPowerOfTwo(int32_t value) {
	float result = log2((float)value);
	return pow(2, (int)ceilf(result));
}

inline uint32_t NextPowerOfTwo(uint32_t n)
{
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n + 1;
}

inline size_t AlignMemory(size_t ptr, size_t alignment)
{
    return (((ptr)+((alignment)-1)) & ~((alignment)-1));
}

inline mat4 ComposeMatrix(const vec3& position, const quat& rotation, const vec3& scale)
{
	mat4 result;
	result = glm::translate(result, position);
	result *= mat4_cast(rotation);
	result = glm::scale(result, scale);
	return result;
}

inline uint32_t FastHash(const void* key, size_t len)
{
	uint32_t result;
	MurmurHash3_x86_32(key, (int)len, 0xdeadbeef, &result);
	return result;
}

inline uint64_t FastHash64(const void* key, size_t len)
{
    return MurmurHash2_x64(key, (int)len, 0xdeadbeef);
}

inline uint64_t FastHash64(const std::string& str)
{
    return MurmurHash2_x64(str.c_str(), (int)str.length(), 0xdeadbeef);
}

inline uint32_t FastHash(const std::string& str)
{
	uint32_t result;
	MurmurHash3_x86_32(str.data(), (int)str.length(), 0xdeadbeef, &result);
	return result;
}

inline uint32_t FastHash(const std::wstring& str)
{
	uint32_t result;
	MurmurHash3_x86_32(str.data(), (int)str.length() * 2, 0xdeadbeef, &result);
	return result;
}

inline uint32_t FastHash(const char* str)
{
    const auto length = strlen(str);
    uint32_t result;
    MurmurHash3_x86_32(str, (int)length * sizeof(char), 0xdeadbeef, &result);
    return result;
}

inline uint32_t FastHash(const wchar_t* str)
{
    const auto length = wcslen(str);
    uint32_t result;
    MurmurHash3_x86_32(str, (int)length * sizeof(wchar_t), 0xdeadbeef, &result);
    return result;
}

inline uint64_t FastHash64(const wchar_t* str)
{
    const auto length = wcslen(str);
    return MurmurHash2_x64(str, (int)length * sizeof(wchar_t), 0xdeadbeef);
}

inline float LogBase(float num, float base)
{
    return log2f(num) / log2f(base);
}

inline vec4 RGBAFromUint(uint32_t color)
{
    return vec4( 
                 ((color >> 24) & 0xff) / 255.0f,
                 ((color >> 16) & 0xff) / 255.0f,
                 ((color >>  8) & 0xff) / 255.0f,
                 ((color      ) & 0xff) / 255.0f
               );
}

inline void Randomize()
{
    std::srand(std::time(nullptr));
}

inline float Random()
{
    return std::rand() / ((float)RAND_MAX + 1);
}