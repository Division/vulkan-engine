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
	lib/imgui/imconfig.h
	lib/imgui/imgui.h
	lib/imgui/imgui.cpp
	lib/imgui/imgui_demo.cpp
	lib/imgui/imgui_draw.cpp
	lib/imgui/imgui_impl_glfw.h
	lib/imgui/imgui_impl_glfw.cpp
	lib/imgui/imgui_internal.h
	lib/imgui/imgui_widgets.cpp
	lib/imgui/imstb_rectpack.h
	lib/imgui/imstb_textedit.h
	lib/imgui/imstb_truetype.h
	lib/ktx/checkheader.c
	lib/ktx/errstr.c
	lib/ktx/filestream.c
	#lib/ktx/glloader.c
	lib/ktx/hashlist.c
	lib/ktx/hashtable.c
	lib/ktx/memstream.c
	lib/ktx/swap.c
	lib/ktx/texture.c
	#lib/ktx/writer.c
	#lib/ktx/writer_v1.c
	#lib/ktx/etcdec.cxx
    #lib/ktx/etcunpack.cxx
	lib/ktx/ktx.h
	lib/ktx/filestream.h
	lib/ktx/gl_format.h
	lib/ktx/gl_funcptrs.h
	lib/ktx/gles1_funcptrs.h
	lib/ktx/gles2_funcptrs.h
	lib/ktx/gles3_funcptrs.h
	lib/ktx/ktxgl.h
	lib/ktx/ktxint.h
	lib/ktx/memstream.h
	lib/ktx/stream.h
	lib/ktx/uthash.h
	lib/ktx/vkloader.c
	lib/ktx/vk_funcs.c
	Engine.h
	Engine.cpp
	Handle.h
	Handle.cpp
	IGame.h
	lib.cpp
	CommonIncludes.h
	utils/NonCopyable.h
	ecs/ECS.h
	ecs/EntityChunks.h
	ecs/TransformGraph.h
	ecs/CommandBuffer.h
	ecs/components/Transform.h
	ecs/components/Entity.h
	ecs/components/MeshRenderer.h
	ecs/components/DrawCall.h
	ecs/components/Light.h
	ecs/components/Light.cpp
	ecs/systems/TransformSystem.h
	ecs/systems/TransformSystem.cpp
	ecs/systems/RendererSystem.h
	ecs/systems/RendererSystem.cpp
	ecs/systems/CullingSystem.h
	ecs/systems/CullingSystem.cpp
	ecs/systems/UpdateDrawCallsSystem.h
	ecs/systems/UpdateDrawCallsSystem.cpp
	memory/Profiler.h
	memory/Profiler.cpp
	memory/Allocator.h
	memory/Allocator.cpp
	memory/PoolAllocator.h
	memory/PoolAllocator.cpp
	memory/Containers.h
	render/device/Resource.h
	render/device/Resource.cpp
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
	render/buffer/DynamicBuffer.h
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
	render/renderer/Synchronization.h
	render/renderer/Synchronization.cpp
	render/renderer/EnvironmentSettings.h
	render/debug/DebugSettings.h
	render/debug/DebugDraw.h
	render/debug/DebugDraw.cpp
	render/debug/DebugUI.h
	render/debug/DebugUI.cpp
	render/debug/Profiler.h
	render/debug/Profiler.cpp
	render/debug/widgets/EngineStats.h
	render/debug/widgets/EngineStats.cpp
	render/debug/widgets/MainWidget.cpp
	render/debug/widgets/EnvironmentEditor.cpp
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
	render/shader/ShaderCompiler.h
	render/shader/ShaderCompiler.cpp
	render/shading/IShadowCaster.h
	render/shading/LightGrid.h
	render/shading/LightGrid.cpp
	render/shading/ShadowMap.h
	render/shading/ShadowMap.cpp
	render/effects/Skybox.h
	render/effects/Skybox.cpp
	render/effects/PostProcess.h
	render/effects/PostProcess.cpp
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
	resources/ResourceCache.h
	resources/ResourceCache.cpp
	resources/HierarchyData.h
	resources/HierarchyData.cpp
	resources/SkinningData.h
	resources/SkinningData.cpp
	resources/TextureResource.h
	resources/TextureResource.cpp
	resources/MultiMesh.h
	resources/MultiMesh.cpp
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
	system/JobSystem.h
	system/JobSystem.cpp
	utils/Math.h
	utils/DataStructures.h
	utils/StringUtils.h
	utils/StringUtils.cpp
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
set(GLFW_INSTALL false)
add_subdirectory(lib/glfw-3.3)
set(EXPORT_TARGETS ${EXPORT_TARGETS} glfw)
set(OPTICK_ENABLED true)
set(OPTICK_USE_VULKAN false)

add_subdirectory(lib/glm)
add_subdirectory(lib/SPIRV-Cross)
add_subdirectory(lib/optick)

add_library(vk_engine ${ENGINE_SOURCES})
target_compile_definitions(vk_engine PRIVATE VK_USE_PLATFORM_WIN32_KHR)
target_include_directories(vk_engine PRIVATE Vulkan::Vulkan)
target_link_libraries(vk_engine glfw)
target_link_libraries(vk_engine Vulkan::Vulkan)
target_link_libraries(vk_engine spirv-cross-core)
target_link_libraries(vk_engine spirv-cross-glsl)
target_link_libraries(vk_engine spirv-cross-cpp)
target_link_libraries(vk_engine spirv-cross-reflect)
target_link_libraries(vk_engine OptickCore)
target_link_libraries(vk_engine ${CMAKE_CURRENT_SOURCE_DIR}/lib/static/dxc/dxcompiler.lib)

# Tests
set(TEST_SOURCES
	tests/test_main.cpp
	tests/test_ecs.cpp
	tests/test_transform_graph.cpp
	tests/test_systems.cpp
	tests/test_transform_system.cpp
	tests/test_containers.cpp
	tests/test_allocators.cpp
	tests/test_job_system.cpp
	tests/test_ktx.cpp
)

add_executable(engine_tests ${TEST_SOURCES})
target_link_libraries(engine_tests vk_engine)