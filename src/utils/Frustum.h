#ifndef H_FRUSTUM
#define H_FRUSTUM

#include "EngineMath.h"

#define MAX_CLIP_PLANES 16

// This code is taken from https://github.com/XProger/OpenLara

struct Frustum {
  mutable vec4 planes[MAX_CLIP_PLANES * 2];   // + buffer for OBB visibility test
  mutable int  start, count;

  void calcPlanes(const mat4 &matrix) {
    start = 0;
    count = 5;
    mat4 m = glm::transpose(matrix);
    planes[0] = vec4(m[3][0] - m[2][0], m[3][1] - m[2][1], m[3][2] - m[2][2], m[3][3] - m[2][3]); // near
    planes[1] = vec4(m[3][0] - m[1][0], m[3][1] - m[1][1], m[3][2] - m[1][2], m[3][3] - m[1][3]); // top
    planes[2] = vec4(m[3][0] - m[0][0], m[3][1] - m[0][1], m[3][2] - m[0][2], m[3][3] - m[0][3]); // right
    planes[3] = vec4(m[3][0] + m[1][0], m[3][1] + m[1][1], m[3][2] + m[1][2], m[3][3] + m[1][3]); // bottom
    planes[4] = vec4(m[3][0] + m[0][0], m[3][1] + m[0][1], m[3][2] + m[0][2], m[3][3] + m[0][3]); // left
    for (int i = 0; i < count; i++) {
      planes[i] /= glm::length(vec3(planes[i]));
    }
  }

  // AABB visibility check
  bool isVisible(const vec3 &min, const vec3 &max, vec4 *planes = nullptr) const {
    if (count < 4) return false;

	if (!planes) {
		planes = &this->planes[start];
	}

    for (int i = 0; i < count; i++) {
      const vec3 &n = vec3(planes[i]);
      const float d = -planes[i].w;

      if (glm::dot(n, max) < d &&
          glm::dot(n, min) < d &&
          glm::dot(n, vec3(min.x, max.y, max.z)) < d &&
          glm::dot(n, vec3(max.x, min.y, max.z)) < d &&
          glm::dot(n, vec3(min.x, min.y, max.z)) < d &&
          glm::dot(n, vec3(max.x, max.y, min.z)) < d &&
          glm::dot(n, vec3(min.x, max.y, min.z)) < d &&
          glm::dot(n, vec3(max.x, min.y, min.z)) < d) {

        return false;
      }
    }

    return true;
  }

  // OBB visibility check
  bool isVisible(const mat4 &matrix, const vec3 &min, const vec3 &max) const {
	vec4 modifiedPlanes[5];
    start = count;
    // transform clip planes (relative)
    mat4 m = glm::inverse(matrix);
    for (int i = 0; i < count; i++) {
      vec4 &p = planes[i];
      vec4 o = m * vec4(vec3(p) * (-p.w), 1.0f);
      vec4 n = m * vec4(vec3(p), 0.0f);
	  modifiedPlanes[i] = vec4(vec3(n), -glm::dot(vec3(n), vec3(o)));
    }
    bool visible = isVisible(min, max, modifiedPlanes);
    start = 0;
    return visible;
  }

  // Sphere visibility check
  bool isVisible(const vec3 &center, float radius) const {
    if (count < 4) { return false; }

    for (int i = 0; i < count; i++) {
      if (glm::dot(vec3(planes[i]), center) + planes[i].w < -radius) {
        return false;
      }
    }

    return true;
  }

};

#endif