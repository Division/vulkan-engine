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
	ecs/ECS.h
	ecs/EntityChunks.h
	ecs/TransformGraph.h
	ecs/components/Transform.h
	ecs/components/Entity.h
	ecs/components/MeshRenderer.h
	ecs/components/CullingData.h
	ecs/components/DrawCall.h
	ecs/systems/TransformSystem.h
	ecs/systems/TransformSystem.cpp
	ecs/systems/RendererSystem.h
	ecs/systems/RendererSystem.cpp
	ecs/systems/CullingSystem.h
	ecs/systems/CullingSystem.cpp
	ecs/systems/UpdateDrawCallsSystem.h
	ecs/systems/UpdateDrawCallsSystem.cpp
	render/device/VkObjects.h
	render/device/VkObjects.cpp
	render/device/VulkanContext.h
	render/device/VulkanContext.cpp
	render/device/VulkanDescriptorCache.h
	render/device/VulkanDescriptorCache.cpp
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
	render/device/Types.h
	render/device/Types.cpp
	render/buffer/VulkanBuffer.h
	render/buffer/VulkanBuffer.cpp
	render/buffer/UniformBuffer.h
	render/material/Material.h
	render/material/Material.cpp
	render/material/MaterialManager.h
	render/material/MaterialManager.cpp
	render/renderer/IRenderer.h
	render/renderer/ICameraParamsProvider.h
	render/renderer/DrawCallManager.h
	render/renderer/DrawCallManager.cpp
	render/renderer/SceneRenderer.h
	render/renderer/SceneRenderer.cpp
	render/renderer/SceneBuffers.h
	render/renderer/SceneBuffers.cpp
	render/renderer/RenderGraph.h
	render/renderer/RenderGraph.cpp
	render/debug/DebugDraw.h
	render/debug/DebugDraw.cpp
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
	render/shader/ShaderCache.h
	render/shader/ShaderCache.cpp
	render/shading/IShadowCaster.h
	render/shading/LightGrid.h
	render/shading/LightGrid.cpp
	render/shading/ShadowMap.h
	render/shading/ShadowMap.cpp
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
	utils/DataStructures.h
	utils/StringUtils.h
	utils/Math.cpp
	utils/MeshGeneration.h
	utils/MeshGeneration.cpp
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

# Tests
set(TEST_SOURCES
	tests/test_main.cpp
	tests/test_ecs.cpp
	tests/test_transform_graph.cpp
	tests/test_systems.cpp
	tests/test_transform_system.cpp
	tests/test_containers.cpp
)

add_executable(engine_tests ${TEST_SOURCES})
target_link_libraries(engine_tests vk_engine)