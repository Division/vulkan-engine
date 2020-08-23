#pragma once

#include "murmurhash/MurmurHash3.h"
#include <math.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>

using namespace glm;

extern const vec3 VectorLeft;
extern const vec3 VectorRight;
extern const vec3 VectorUp;
extern const vec3 VectorDown;
extern const vec3 VectorForward;
extern const vec3 VectorBack;

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

inline int32_t getPowerOfTwo(int32_t value) {
	float result = log2((float)value);
	return pow(2, (int)ceilf(result));
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

inline float LogBase(float num, float base)
{
    return log2f(num) / log2f(base);
}