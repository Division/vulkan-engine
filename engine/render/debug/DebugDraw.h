#pragma once

#include "CommonIncludes.h"
#include "render/shader/ShaderBufferStruct.h"
#include "ecs/components/DrawCall.h"

class Mesh;

namespace Device
{
    class ShaderCache;
    class ShaderProgram;
}

namespace render {

    class DebugDraw {
    public:
        DebugDraw(Device::ShaderCache& shader_cache);

        void DrawLine(const vec3 &p1, const vec3 &p2, const vec4 &color);
        void DrawPoint(const vec3 &p, const vec3 &color, float size = 5.0f);
        void DrawFrustum(mat4 view_projection, vec4 color = vec4(1, 0, 0, 1));
        void DrawAABB(const vec3 &min, const  vec3 &max, const vec4 &color);
        void DrawAABB(const AABB &bounds, const vec4 &color);
        void DrawOBB(const OBB &bounds, const vec4 &color);

        auto& GetLineDrawCalls() const { return line_draw_calls; }
        auto& GetPointDrawCalls() const { return point_draw_calls; }

        void Update();
        
        Device::ShaderProgram* GetShader() const { return shader; }

    private:
        int current_mesh_index = 0;
        std::vector<vec3> lines;
        std::vector<vec3> points;
        std::vector<vec4> line_colors;
        std::vector<vec4> point_colors;

        std::unique_ptr<Mesh> line_meshes[2]; // two meshes to swap
        std::unique_ptr<Mesh> point_meshes[2];
        std::vector<ECS::components::DrawCall> line_draw_calls;
        std::vector<ECS::components::DrawCall> point_draw_calls;
        Device::ShaderProgram* shader;
    };

}