//
// Created by Sidorenko Nikita on 4/3/18.
//

#include "Camera.h"
#include "Engine.h"
#include "render/device/VulkanContext.h"
#include "render/device/VulkanSwapchain.h"

void Camera::UpdateProjection() 
{
    auto* swapchain = Engine::Get()->GetVulkanContext()->GetSwapchain();
    float width = swapchain->GetWidth();
    float height = swapchain->GetHeight();
    float aspect = width / height;
    switch (mode) {
    case Mode::Perspective:
        projection_matrix = glm::perspective(glm::radians(fov), aspect, zMin, zMax);
        break;

    case Mode::Ortho: {
        float halfHeight = orthographic_size / 2.0f;
        float halfWidth = aspect * halfHeight;
        projection_matrix = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, zMin, zMax);
        break;
    }
    case Mode::UI:
        float halfHeight = height / 2.0f;
        float halfWidth = width / 2.0f;
        projection_matrix = glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, -1.0f, 1.0f);
        break;
    }
}

void Camera::UpdateView() 
{
    view_matrix = glm::inverse(transform.local_to_world);
}

void Camera::UpdateViewport() 
{
	auto* swapchain = Engine::Get()->GetVulkanContext()->GetSwapchain();
	viewport = vec4(0, 0, swapchain->GetWidth(), swapchain->GetHeight());
}

void Camera::Update() 
{
    transform.local_to_world = ComposeMatrix(transform.position, transform.rotation, transform.scale);
    UpdateViewport();
    UpdateProjection();
    UpdateView();
    view_projection_matrix = projection_matrix * view_matrix;
    frustum.calcPlanes(view_projection_matrix);
}

std::pair<vec3, vec3> Camera::GetMouseRay(vec2 mouse_position)
{
    const vec3 start = Transform().WorldPosition();
    const auto screen_size = Engine::Get()->GetScreenSize();
    const glm::vec3 world_pos = glm::unProject(vec3(vec2(mouse_position.x, screen_size.y - mouse_position.y), 1.0f), view_matrix, projection_matrix, viewport);
    const glm::vec3 dir = glm::normalize(glm::vec3(world_pos) - start);
     
    return std::make_pair(start, dir);
}