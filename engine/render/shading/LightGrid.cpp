#include <functional>

#include "LightGrid.h"
#include "objects/Camera.h"
#include "system/Logging.h"
#include "render/renderer/ICameraParamsProvider.h"
#include "render/device/VulkanContext.h"
#include "render/shader/ShaderResource.h"
#include "render/buffer/DynamicBuffer.h"
#include "render/debug/DebugDraw.h"
#include "ecs/components/Light.h"

using namespace Device;

namespace render {

const uint32_t MAX_LIGHTS = 100;
const uint32_t LIGHTS_SIZE = ((uint32_t)ceilf(MAX_LIGHTS * sizeof(ShaderBufferStruct::Light) / 256.0f)) * 256;
const uint32_t PROJECTORS_SIZE = ((uint32_t)ceilf(MAX_LIGHTS * sizeof(ShaderBufferStruct::Light) / 256.0f)) * 256;

LightGrid::LightGrid()
{	
	auto* context = Engine::GetVulkanContext();
	context->AddRecreateSwapchainCallback(std::bind(&LightGrid::OnRecreateSwapchain, this, std::placeholders::_1, std::placeholders::_2));
    projectors[0] = std::make_unique<DynamicBuffer<ShaderBufferStruct::Projector>>("Projectors", sizeof(ShaderBufferStruct::Projector), BufferType::Uniform, true);
    lights[0] = std::make_unique<DynamicBuffer<ShaderBufferStruct::Light>>("Lights", sizeof(ShaderBufferStruct::Light), BufferType::Uniform, true);
    light_index[0] = std::make_unique<DynamicBuffer<char>>("LightIndex", 1, BufferType::Storage, true);
    light_grid[0] = std::make_unique<DynamicBuffer<char>>("LightClusters", 1, BufferType::Uniform, true);
}

float LightGrid::GetSliceMaxDepth(uint32_t slice)
{
    return CLUSTER_NEAR * powf((float)CLUSTER_FAR / (float)CLUSTER_NEAR, (float)(slice + 1) / (float)CLUSTER_COUNT_DEPTH);
}

uint32_t LightGrid::GetSliceIndex(float depth)
{
    float index = LogBase(depth / CLUSTER_NEAR, (float)CLUSTER_FAR / (float)CLUSTER_NEAR) * CLUSTER_COUNT_DEPTH;
    return (uint32_t)(std::max(0.0f, index));
}

void LightGrid::UpdateSlices(mat4 projection)
{
    if (last_matrix == projection)
        return;
    
    last_matrix = projection;

    auto inv = glm::inverse(projection);
    vec4 quad_near[4] = {
        inv * vec4(-1, -1, 0, 1),
        inv * vec4(1, -1, 0, 1),
        inv * vec4(-1, 1, 0, 1),
        inv * vec4(1, 1, 0, 1),
    };

    vec4 quad_far[4] = {
        inv * vec4(-1, -1, 1, 1),
        inv * vec4(1, -1, 1, 1),
        inv * vec4(-1, 1, 1, 1),
        inv * vec4(1, 1, 1, 1),
    };

    for (int i = 0; i < 4; i++) 
    {
        quad_near[i] /= quad_near[i].w;
        quad_far[i] /= quad_far[i].w;
    }

    auto near_min = quad_near[0];
    auto near_max = quad_near[3];
    auto far_min = quad_far[0];
    auto far_max = quad_far[3];

    float zNear = quad_near[0].z;
    float zFar = quad_far[0].z;
    auto get_hypotenuse_length = [zNear](float depth, float cos_a) { return std::abs(std::min((depth - zNear) / cos_a, 0.0f)); };

    slices.resize(CLUSTER_COUNT_DEPTH);
    float start_depth = 0;
    for (int slice_index = 0; slice_index < CLUSTER_COUNT_DEPTH; slice_index++)
    {
        auto& slice = slices[slice_index];
        float end_depth = -GetSliceMaxDepth(slice_index); // Making depth negative same as z axis

        for (int j = 0; j < CLUSTER_COUNT_Y; j++)
            for (int i = 0; i < CLUSTER_COUNT_X; i++)
            {
                vec4 cluster_min_near = near_min + (near_max - near_min) * vec4(i / (float)CLUSTER_COUNT_X, j / (float)CLUSTER_COUNT_Y, 1, 0);
                vec4 cluster_max_near = near_min + (near_max - near_min) * vec4((i + 1) / (float)CLUSTER_COUNT_X, (j + 1) / (float)CLUSTER_COUNT_Y, 1, 0);
                vec4 cluster_min_far = far_min + (far_max - far_min) * vec4(i / (float)CLUSTER_COUNT_X, j / (float)CLUSTER_COUNT_Y, 1, 0);
                vec4 cluster_max_far = far_min + (far_max - far_min) * vec4((i + 1) / (float)CLUSTER_COUNT_X, (j + 1) / (float)CLUSTER_COUNT_Y, 1, 0);
                auto cluster_near_delta = cluster_max_near - cluster_min_near;
                auto cluster_far_delta = cluster_max_far - cluster_min_far;

                auto cluster_index = i + j * CLUSTER_COUNT_X;
                auto& cluster = slice.clusters[cluster_index];
                auto& cluster_info = slice.clusters_info[cluster_index];

                std::array<vec3, 4> quad_intermediate_near;
                std::array<vec3, 4> quad_intermediate_far;
                std::array<vec3, 4> direction_intermediate;

                quad_intermediate_near[0] = cluster_min_near;
                quad_intermediate_near[1] = vec3(cluster_min_near) + vec3(cluster_near_delta.x, 0, 0);
                quad_intermediate_near[2] = vec3(cluster_min_near) + vec3(0, cluster_near_delta.y, 0);
                quad_intermediate_near[3] = cluster_min_near + cluster_near_delta;
                quad_intermediate_far[0] = cluster_min_far;
                quad_intermediate_far[1] = vec3(cluster_min_far) + vec3(cluster_far_delta.x, 0, 0);
                quad_intermediate_far[2] = vec3(cluster_min_far) + vec3(0, cluster_far_delta.y, 0);
                quad_intermediate_far[3] = cluster_min_far + cluster_far_delta;

                for (size_t c = 0; c < 4; c++)
                {
                    vec3 direction = normalize(quad_intermediate_far[c] - quad_intermediate_near[c]);
                    float cos_a = dot(direction, vec3(0, 0, -1)); // z axis direction is negative
                    cluster_info.view_space_frustum_corners[c] = quad_intermediate_near[c] + get_hypotenuse_length(start_depth, cos_a) * direction;
                    cluster_info.view_space_frustum_corners[c + 4] = quad_intermediate_near[c] + get_hypotenuse_length(end_depth, cos_a) * direction;
                }

                /*std::array<vec4, 8> ndc_corners;
                for (int c = 0; c < cluster_info.view_space_frustum_corners.size(); c++)
                {
                    ndc_corners[c] = projection * vec4(cluster_info.view_space_frustum_corners[c], 1);
                    ndc_corners[c] /= ndc_corners[c].w;
                }*/
                
                cluster_info.viewspace_aabb.min = cluster_info.view_space_frustum_corners[0];
                cluster_info.viewspace_aabb.max = cluster_info.view_space_frustum_corners[0];
                for (auto& corner : cluster_info.view_space_frustum_corners)
                    cluster_info.viewspace_aabb.expand(corner);
            }

        start_depth = end_depth;
    }
}

void LightGrid::DrawDebugClusters(mat4 model_matrix, vec4 color)
{
    auto* debug_draw = Engine::Get()->GetDebugDraw();

    for (int d = 0; d < CLUSTER_COUNT_DEPTH; d++)
    {
        auto& slice = slices[d];
        for (int i = 0; i < slice.clusters.size(); i++)
        {
            auto& verts = slice.clusters_info[i].view_space_frustum_corners;
            std::array<vec3, 8> transformed_verts;
            for (int j = 0; j < 8; j++)
                transformed_verts[j] = model_matrix * vec4(verts[j], 1);

            /*for (int j = 0; j < 4; j++)
                debug_draw->DrawLine(transformed_verts[j], transformed_verts[j + 4], color);*/
            
            for (int j = 1; j < 2; j++)
            {
                debug_draw->DrawLine(transformed_verts[0 + j * 4], transformed_verts[1 + j * 4], color);
                debug_draw->DrawLine(transformed_verts[1 + j * 4], transformed_verts[3 + j * 4], color);
                debug_draw->DrawLine(transformed_verts[3 + j * 4], transformed_verts[2 + j * 4], color);
                debug_draw->DrawLine(transformed_verts[2 + j * 4], transformed_verts[0 + j * 4], color);
            }
        }
    }
}

template <typename T>
void ResizeBuffer(std::unique_ptr<DynamicBuffer<T>> buffer[2], size_t size, bool is_storage)
{
	if (!buffer[0] || buffer[0]->GetSize() < size)
	{
		buffer[0] = std::make_unique<DynamicBuffer<T>>(buffer[0]->GetName(), std::max(size, buffer[0]->GetElementSize()) , is_storage ? BufferType::Storage : BufferType::Uniform, false);
	}
}

AABB GetViewSpaceAABB(const OBB& src_obb, mat4 view_matrix)
{
    std::array<vec4, 8> verts;
    auto delta = src_obb.max - src_obb.min;
    auto combined_matrix = view_matrix * src_obb.matrix;
    auto min = vec4(src_obb.min, 1);

    verts[0] = combined_matrix * min;
    verts[1] = combined_matrix * (min + vec4(delta.x, 0, 0, 0));
    verts[2] = combined_matrix * (min + vec4(delta.x, delta.y, 0, 0));
    verts[3] = combined_matrix * (min + vec4(0, delta.y, 0, 0));
    verts[4] = combined_matrix * (min + vec4(0, 0, delta.z, 0));
    verts[5] = combined_matrix * (min + vec4(delta.x, 0, delta.z, 0));
    verts[6] = combined_matrix * (min + vec4(delta.x, delta.y, delta.z, 0));
    verts[7] = combined_matrix * (min + vec4(0, delta.y, delta.z, 0));

    AABB aabb(verts[0], verts[0]);
    for (int i = 1; i < verts.size(); i++)
        aabb.expand(verts[i]);

    return aabb;
}

void LightGrid::appendLights(const std::vector<Scene::SceneLightData> &light_list, ICameraParamsProvider* camera, float shadowmap_atlas_size)
{
    OPTICK_EVENT();

    auto size = light_list.size() * sizeof(ShaderBufferStruct::Light);
    ResizeBuffer(lights, std::max(size, sizeof(ShaderBufferStruct::Light)), false);
    lights[0]->Map();

    light_aabb.resize(light_list.size());
    for (int i = 0; i < light_list.size(); i++) 
    {
	    auto& light = light_list[i];
        auto lightData = light.light->GetShaderStruct(light.position, light.direction, shadowmap_atlas_size);
        lights[0]->Append(lightData);
        light_aabb[i] = GetViewSpaceAABB(light.transform->GetOBB(), camera->cameraViewMatrix());
    }

    lights[0]->Unmap();

    for (int slice_index = 0; slice_index < slices.size(); slice_index++)
    {
        auto& slice = slices[slice_index];
        slice.indices.clear();
        for (int i = 0; i < slice.clusters.size(); i++)
        {
            auto& cluster = slice.clusters[i];
            auto& cluster_info = slice.clusters_info[i];
            cluster.pointLightCount = 0;
            cluster.spotLightCount = 0;
            cluster.projectorCount = 0;
            cluster.decalCount = 0;
            cluster.offset = 0;

            for (int l = 0; l < light_list.size(); l++)
            {
                if (light_list[l].light->type == ECS::components::Light::Type::Point)
                {
                    if (cluster_info.viewspace_aabb.intersectsAABB(light_aabb[l]))
                    {
                        cluster.pointLightCount += 1;
                        slice.indices.push_back(l);
                    }
                }
            }

            for (int l = 0; l < light_list.size(); l++)
            {
                if (light_list[l].light->type == ECS::components::Light::Type::Spot)
                {
                    if (cluster_info.viewspace_aabb.intersectsAABB(light_aabb[i]))
                    {
                        cluster.projectorCount += 1;
                        slice.indices.push_back(l);
                    }
                }
            }
        }
    }
}

// Upload grid data into the GPU buffers
void LightGrid::upload() 
{
    OPTICK_EVENT();
    auto light_grid_size = CLUSTER_COUNT_X * CLUSTER_COUNT_Y * CLUSTER_COUNT_DEPTH * sizeof(LightGridStruct);

	ResizeBuffer(light_grid, light_grid_size, true);

	auto* grid_buffer_pointer = (LightGridStruct *)light_grid[0]->Map();
    uint32_t index_count = 0;

    for (int slice_index = 0; slice_index < slices.size(); slice_index++)
    {
        auto& slice = slices[slice_index];
        
        for (auto& cluster : slice.clusters)
        { 
            cluster.offset += index_count;
            index_count += cluster.pointLightCount + cluster.spotLightCount + cluster.projectorCount + cluster.decalCount;
        }

        memcpy((uint8_t*)grid_buffer_pointer + sizeof(slice.clusters) * slice_index, slice.clusters.data(), sizeof(slice.clusters));
    }

    light_grid[0]->SetUploadSize(light_grid_size);
	light_grid[0]->Unmap();
    
    if (!index_count)
        return;

	ResizeBuffer(light_index, index_count * sizeof(uint32_t), true);
	auto light_index_pointer = (uint8_t*)light_index[0]->Map();

    for (int slice_index = 0; slice_index < slices.size(); slice_index++)
    {
        auto& slice = slices[slice_index];
        size_t index_data_size = slice.indices.size() * sizeof(uint32_t);
        memcpy((uint8_t*)light_index_pointer, slice.indices.data(), index_data_size);
        light_index_pointer += index_data_size;
    }

    light_index[0]->SetUploadSize(index_count * sizeof(uint32_t));
	light_index[0]->Unmap();
}

void LightGrid::OnRecreateSwapchain(int32_t width, int32_t height)
{
}

}