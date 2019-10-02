cmake_minimum_required(VERSION 3.9)
project(vk_engine)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
#SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -M)

set(ENGINE_SOURCES 
	main.cpp
	src/lib/VulkanMemoryAllocator/vk_mem_alloc.h
	src/lib/murmurhash/MurmurHash3.h
	src/lib/murmurhash/MurmurHash3.cpp
	src/Engine.h
	src/Engine.cpp
	src/IGame.h
	src/lib.cpp
	src/CommonIncludes.h
	src/utils/NonCopyable.h
	src/render/DataStructures.h
	src/render/device/Device.h
	src/render/device/VkObjects.h
	src/render/device/VkObjects.cpp
	src/render/device/Device.cpp
	src/render/device/VulkanContext.h
	src/render/device/VulkanContext.cpp
	src/render/device/VulkanUploader.h
	src/render/device/VulkanUploader.cpp
	src/render/device/VulkanRenderPass.h
	src/render/device/VulkanRenderPass.cpp
	src/render/device/VulkanRenderTarget.h
	src/render/device/VulkanRenderTarget.cpp
	src/render/device/VulkanSwapchain.h
	src/render/device/VulkanSwapchain.cpp
	src/render/device/VulkanPipeline.h
	src/render/device/VulkanPipeline.cpp
	src/render/device/VulkanUtils.h
	src/render/device/VulkanUtils.cpp
	src/render/device/VulkanRenderState.h
	src/render/device/VulkanRenderState.cpp
	src/render/device/VulkanCaps.h
	src/render/buffer/VulkanBuffer.h
	src/render/buffer/VulkanBuffer.cpp
	src/render/buffer/UniformBuffer.h
	src/render/pass/RenderPass.h
	src/render/pass/RenderPass.cpp
	src/render/material/Material.h
	src/render/material/Material.cpp
	src/render/renderer/IRenderer.h
	src/render/renderer/ICameraParamsProvider.h
	src/render/renderer/DrawCall.h
	src/render/renderer/DrawCall.cpp
	src/render/renderer/SceneRenderer.h
	src/render/renderer/SceneRenderer.cpp
	src/render/renderer/SceneBuffers.h
	src/render/renderer/SceneBuffers.cpp
	src/render/renderer/RenderOperation.h
	src/render/shader/Shader.h
	src/render/shader/Shader.cpp
	src/render/shader/ReflectionInfo.h
	src/render/shader/ReflectionInfo.cpp
	src/render/shader/ShaderBindings.h
	src/render/shader/ShaderBindings.cpp
	src/render/shader/ShaderDefines.h
	src/render/shader/ShaderDefines.cpp
	src/render/shader/ShaderResource.h
	src/render/shader/ShaderResource.cpp
	src/render/shader/ShaderBindings.h
	src/render/shader/ShaderBindings.cpp
	src/render/shader/ShaderCaps.h
	src/render/shading/IShadowCaster.h
	src/render/mesh/Mesh.h
	src/render/mesh/Mesh.cpp
	src/render/texture/Texture.h
	src/render/texture/Texture.cpp
	src/render/texture/SpriteSheet.h
	src/render/texture/SpriteSheet.cpp
	src/scene/AnimationController.h
	src/scene/AnimationController.cpp
	src/scene/GameObject.h
	src/scene/GameObject.cpp
	src/scene/Scene.h
	src/scene/Scene.cpp
	src/scene/Transform.h
	src/scene/Transform.cpp
	src/objects/Camera.h
	src/objects/Camera.cpp
	src/objects/LightObject.h
	src/objects/LightObject.cpp
	src/objects/MeshObject.h
	src/objects/MeshObject.cpp
	src/objects/Projector.h
	src/objects/Projector.cpp
	src/objects/SkinnedMeshObject.h
	src/objects/SkinnedMeshObject.cpp
	src/resources/ModelBundle.h
	src/resources/ModelBundle.cpp
	src/resources/HierarchyData.h
	src/resources/HierarchyData.cpp
	src/resources/SkinningData.h
	src/resources/SkinningData.cpp
	src/loader/TextureLoader.h
	src/loader/TextureLoader.cpp
	src/loader/FileLoader.h
	src/loader/FileLoader.cpp
	src/loader/HierarchyLoader.h
	src/loader/HierarchyLoader.cpp
	src/loader/SpritesheetLoader.h
	src/loader/SpritesheetLoader.cpp
	src/loader/ModelLoader.h
	src/loader/ModelLoader.cpp
	src/loader/ModelLoaderUtils.h
	src/loader/ModelLoaderUtils.cpp
	src/system/Logging.h
	src/system/Logging.cpp
	src/system/utils.h
	src/system/Input.h
	src/system/Input.cpp
	src/utils/Math.h
	src/utils/Math.cpp
	src/utils/Pool.h
	src/utils/CapsSet.h
	src/utils/Frustum.h
)

set(GAME_SOURCES
	game/Game.h
	game/Game.cpp
	game/level/Level.h
	game/level/Level.cpp
	game/objects/FollowCamera.h
	game/objects/FollowCamera.cpp
	game/objects/PlayerController.h
	game/objects/PlayerController.cpp
)
 
include_directories(src)
include_directories(src/lib)

find_package(Vulkan REQUIRED)

set(GLFW_BUILD_DOCS false)
set(GLFW_BUILD_EXAMPLES false)
set(GLFW_BUILD_TESTS false)
add_subdirectory(src/lib/glfw-3.3)
set(EXPORT_TARGETS ${EXPORT_TARGETS} glfw)

add_subdirectory(src/lib/glm)
add_subdirectory(src/lib/SPIRV-Cross)

add_executable(vk_engine ${ENGINE_SOURCES} ${GAME_SOURCES})
target_compile_definitions(vk_engine PRIVATE VK_USE_PLATFORM_WIN32_KHR)
target_include_directories(vk_engine PRIVATE Vulkan::Vulkan)
target_link_libraries(vk_engine glfw)
target_link_libraries(vk_engine Vulkan::Vulkan)
target_link_libraries(vk_engine spirv-cross-core)
target_link_libraries(vk_engine spirv-cross-glsl)
target_link_libraries(vk_engine spirv-cross-cpp)
target_link_libraries(vk_engine spirv-cross-reflect)
