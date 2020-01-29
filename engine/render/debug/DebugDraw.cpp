//
// Created by Sidorenko Nikita on 2/15/18.
//

#include "DebugDraw.h"
#include "render/mesh/Mesh.h"
#include "utils/MeshGeneration.h"
#include "Engine.h"
#include "render/shader/ShaderCache.h"

namespace core { namespace render {

    using namespace ECS;

    DebugDraw::DebugDraw(Device::ShaderCache& shader_cache) 
    {
        shader = shader_cache.GetShaderProgram(L"shaders/debug_draw.vert", L"shaders/debug_draw.frag");

        line_meshes[0] = std::make_unique<Mesh>(true, 2, false);
        line_meshes[1] = std::make_unique<Mesh>(true, 2, false);
        point_meshes[0] = std::make_unique<Mesh>(true, 1, false);
        point_meshes[1] = std::make_unique<Mesh>(true, 1, false);
    }

    void DebugDraw::DrawLine(const vec3 &p1, const vec3 &p2, const vec4 &color) {
        lines.push_back(p1);
        lines.push_back(p2);
        line_colors.push_back(color);
        line_colors.push_back(color);
    }

    void DebugDraw::DrawPoint(const vec3 &p, const vec3 &color, float size) 
    {
        points.push_back(p);
        point_colors.push_back(vec4(color, size));
    }

    void DebugDraw::DrawFrustum(mat4 viewProjection, vec4 color) 
    {
        auto inv = glm::inverse(viewProjection);
        vec4 quad1[4] = {
            inv * vec4(1, -1, -1, 1),
            inv * vec4(-1, -1, -1, 1),
            inv * vec4(-1, 1, -1, 1),
            inv * vec4(1, 1, -1, 1),
        };

        vec4 quad2[4] = {
            inv * vec4(1, -1, 1, 1),
            inv * vec4(-1, -1, 1, 1),
            inv * vec4(-1, 1, 1, 1),
            inv * vec4(1, 1, 1, 1),
        };

        for (int i = 0; i < 4; i++) 
        {
            quad1[i] /= quad1[i].w;
            quad2[i] /= quad2[i].w;
        }

        for (int i = 0; i < 4; i++) 
        {
            DrawPoint(quad1[i], vec3(color), 10);
            DrawLine(quad1[i], quad1[(i + 1) % 4], color);
            DrawLine(quad2[i], quad2[(i + 1) % 4], color);
            DrawLine(quad1[i], quad2[i], color);
        }
    }

    void DebugDraw::DrawAABB(const vec3 &min, const  vec3 &max, const vec4 &color) 
    {
        vec3 size = max - min;
        const vec3 vertices[] = {
            min,
            min + size * vec3(0, 0, 1),
            min + size * vec3(1, 0, 1),
            min + size * vec3(1, 0, 0),
            max - size * vec3(1, 0, 1),
            max - size * vec3(1, 0, 0),
            max,
            max - size * vec3(0, 0, 1),
        };

        for (int i = 0; i < 4; i++) {
            DrawLine(vertices[i], vertices[(i + 1) % 4], color);
            DrawLine(vertices[i + 4], vertices[(i + 1) % 4 + 4], color);
            DrawLine(vertices[i], vertices[i + 4], color);
        }
    }

    void DebugDraw::DrawAABB(const AABB &bounds, const vec4 &color) 
    {
        DrawAABB(bounds.min, bounds.max, color);
    }

    void DebugDraw::DrawOBB(const OBB &bounds, const vec4 &color) 
    {
        vec3 size = bounds.max - bounds.min;
        vec3 min = bounds.min;
        vec3 max = bounds.max;
        vec3 vertices[] = {
            min,
            min + size * vec3(0, 0, 1),
            min + size * vec3(1, 0, 1),
            min + size * vec3(1, 0, 0),
            max - size * vec3(1, 0, 1),
            max - size * vec3(1, 0, 0),
            max,
            max - size * vec3(0, 0, 1),
        };

        for (vec3 &v : vertices) 
        {
            v = vec3(bounds.matrix * vec4(v, 1));
        }

        for (int i = 0; i < 4; i++) 
        {
            DrawLine(vertices[i], vertices[(i + 1) % 4], color);
            DrawLine(vertices[i + 4], vertices[(i + 1) % 4 + 4], color);
            DrawLine(vertices[i], vertices[i + 4], color);
        }
    }

    void DebugDraw::Update() 
    {
        current_mesh_index = (current_mesh_index + 1) % 2;
        auto& line_mesh = line_meshes[current_mesh_index];
        auto& point_mesh = point_meshes[current_mesh_index];

        line_draw_calls.clear();
        point_draw_calls.clear();

        if (!lines.empty()) 
        {
            line_mesh->setVertices(lines);
            line_mesh->setColors(line_colors);
            line_mesh->createBuffer();

            components::DrawCall draw_call;
            draw_call.shader = shader;
            draw_call.mesh = line_mesh.get();
            draw_call.descriptor_set = nullptr;
            line_draw_calls.push_back(draw_call);
        }

      
      if (!points.empty()) {
        point_mesh->setVertices(points);
        point_mesh->setColors(point_colors);
        point_mesh->createBuffer();

        components::DrawCall draw_call;
        draw_call.shader = shader;
        draw_call.mesh = point_mesh.get();
        draw_call.descriptor_set = nullptr;
        point_draw_calls.push_back(draw_call);
      }

        lines.clear();
        points.clear();
        line_colors.clear();
        point_colors.clear();
    }

} }