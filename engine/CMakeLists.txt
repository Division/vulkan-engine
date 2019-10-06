cmake_minimum_required(VERSION 3.14)
cmake_policy(SET CMP0079 NEW)
cmake_policy(SET CMP0002 NEW)
project(vk_engine)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "cmake")
#SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -M)

set(ENGINE_SOURCES 
	lib/VulkanMemoryAllocator/vk_mem_alloc.h
	lib/murmurhash/MurmurHash3.h
	lib/murmurhash/MurmurHash3.cpp
	Engine.h
	Engine.cpp
	IGame.h
	lib.cpp
	CommonIncludes.h
	utils/NonCopyable.h
	render/DataStructures.h
	render/device/VkObjects.h
	render/device/VkObjects.cpp
	render/device/VulkanContext.h
	render/device/VulkanContext.cpp
	render/device/VulkanUploader.h
	render/device/VulkanUploader.cpp
	render/device/VulkanRenderPass.h
	render/device/VulkanRenderPass.cpp
	render/device/VulkanRenderTarget.h
	render/device/VulkanRenderTarget.cpp
	render/device/VulkanSwapchain.h
	render/device/VulkanSwapchain.cpp
	render/device/VulkanPipeline.h
	render/device/VulkanPipeline.cpp
	render/device/VulkanUtils.h
	render/device/VulkanUtils.cpp
	render/device/VulkanRenderState.h
	render/device/VulkanRenderState.cpp
	render/device/VulkanCaps.h
	render/buffer/VulkanBuffer.h
	render/buffer/VulkanBuffer.cpp
	render/buffer/UniformBuffer.h
	render/pass/RenderPass.h
	render/pass/RenderPass.cpp
	render/material/Material.h
	render/material/Material.cpp
	render/renderer/IRenderer.h
	render/renderer/ICameraParamsProvider.h
	render/renderer/DrawCall.h
	render/renderer/DrawCall.cpp
	render/renderer/SceneRenderer.h
	render/renderer/SceneRenderer.cpp
	render/renderer/SceneBuffers.h
	render/renderer/SceneBuffers.cpp
	render/renderer/RenderOperation.h
	render/shader/Shader.h
	render/shader/Shader.cpp
	render/shader/ReflectionInfo.h
	render/shader/ReflectionInfo.cpp
	render/shader/ShaderBindings.h
	render/shader/ShaderBindings.cpp
	render/shader/ShaderDefines.h
	render/shader/ShaderDefines.cpp
	render/shader/ShaderResource.h
	render/shader/ShaderResource.cpp
	render/shader/ShaderBindings.h
	render/shader/ShaderBindings.cpp
	render/shader/ShaderCaps.h
	render/shading/IShadowCaster.h
	render/mesh/Mesh.h
	render/mesh/Mesh.cpp
	render/texture/Texture.h
	render/texture/Texture.cpp
	render/texture/SpriteSheet.h
	render/texture/SpriteSheet.cpp
	scene/AnimationController.h
	scene/AnimationController.cpp
	scene/GameObject.h
	scene/GameObject.cpp
	scene/Scene.h
	scene/Scene.cpp
	scene/Transform.h
	scene/Transform.cpp
	objects/Camera.h
	objects/Camera.cpp
	objects/LightObject.h
	objects/LightObject.cpp
	objects/MeshObject.h
	objects/MeshObject.cpp
	objects/Projector.h
	objects/Projector.cpp
	objects/SkinnedMeshObject.h
	objects/SkinnedMeshObject.cpp
	resources/ModelBundle.h
	resources/ModelBundle.cpp
	resources/HierarchyData.h
	resources/HierarchyData.cpp
	resources/SkinningData.h
	resources/SkinningData.cpp
	loader/TextureLoader.h
	loader/TextureLoader.cpp
	loader/FileLoader.h
	loader/FileLoader.cpp
	loader/HierarchyLoader.h
	loader/HierarchyLoader.cpp
	loader/SpritesheetLoader.h
	loader/SpritesheetLoader.cpp
	loader/ModelLoader.h
	loader/ModelLoader.cpp
	loader/ModelLoaderUtils.h
	loader/ModelLoaderUtils.cpp
	system/Logging.h
	system/Logging.cpp
	system/utils.h
	system/Input.h
	system/Input.cpp
	utils/Math.h
	utils/Math.cpp
	utils/Pool.h
	utils/CapsSet.h
	utils/Frustum.h
)
 
include_directories(./ lib)

find_package(Vulkan REQUIRED)

set(GLFW_BUILD_DOCS false)
set(GLFW_BUILD_EXAMPLES false)
set(GLFW_BUILD_TESTS false)
add_subdirectory(lib/glfw-3.3)
set(EXPORT_TARGETS ${EXPORT_TARGETS} glfw)

add_subdirectory(lib/glm)
add_subdirectory(lib/SPIRV-Cross)

add_library(vk_engine ${ENGINE_SOURCES})
target_compile_definitions(vk_engine PRIVATE VK_USE_PLATFORM_WIN32_KHR)
target_include_directories(vk_engine PRIVATE Vulkan::Vulkan)
target_link_libraries(vk_engine glfw)
target_link_libraries(vk_engine Vulkan::Vulkan)
target_link_libraries(vk_engine spirv-cross-core)
target_link_libraries(vk_engine spirv-cross-glsl)
target_link_libraries(vk_engine spirv-cross-cpp)
target_link_libraries(vk_engine spirv-cross-reflect)
