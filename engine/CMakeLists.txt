cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
cmake_policy(SET CMP0079 NEW)
cmake_policy(SET CMP0002 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
project(vk_engine)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "cmake")

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
	lib/half/half.h
	lib/half/half.cpp
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
	ecs/EntityTemplate.h
	ecs/EntityTemplate.cpp
	ecs/components/Static.h
	ecs/components/Transform.h
	ecs/components/Entity.h
	ecs/components/MultiMeshRenderer.h
	ecs/components/DrawCall.h
	ecs/components/Light.h
	ecs/components/Light.cpp
	ecs/components/Physics.h
	ecs/components/AnimationController.h
	ecs/components/BoneAttachment.h
	ecs/systems/TransformSystem.h
	ecs/systems/TransformSystem.cpp
	ecs/systems/RendererSystem.h
	ecs/systems/RendererSystem.cpp
	ecs/systems/PhysicsSystem.h
	ecs/systems/PhysicsSystem.cpp
	ecs/systems/CullingSystem.h
	ecs/systems/CullingSystem.cpp
	ecs/systems/UpdateDrawCallsSystem.h
	ecs/systems/UpdateDrawCallsSystem.cpp
	ecs/systems/SkinningSystem.h
	ecs/systems/SkinningSystem.cpp
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
	render/buffer/ConstantBuffer.h
	render/material/Material.h
	render/material/Material.cpp
	render/renderer/RenderModeUtils.h
	render/renderer/RenderModeUtils.cpp
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
	render/debug/ImGUIHelper.h
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
	render/effects/Bloom.h
	render/effects/Bloom.cpp
	render/effects/Blur.h
	render/effects/Blur.cpp
	render/mesh/Mesh.h
	render/mesh/Mesh.cpp
	render/texture/Texture.h
	render/texture/Texture.cpp
	render/animation/SkeletalAnimation.h
	render/animation/SkeletalAnimation.cpp
	scene/Scene.h
	scene/Scene.cpp
	scene/Physics.h
	scene/Physics.cpp
	scene/PhysicsHelper.h
	scene/PhysicsHelper.cpp
	objects/Camera.h
	objects/Camera.cpp
	resources/ResourceCache.h
	resources/ResourceCache.cpp
	resources/TextureResource.h
	resources/TextureResource.cpp
	resources/MultiMesh.h
	resources/MultiMesh.cpp
	resources/PhysCollider.h
	resources/PhysCollider.cpp
	resources/MaterialResource.h
	resources/MaterialResource.cpp
	resources/EntityResource.h
	resources/EntityResource.cpp
	resources/SkeletonResource.h
	resources/SkeletonResource.cpp
	resources/SkeletalAnimationResource.h
	resources/SkeletalAnimationResource.cpp
	loader/TextureLoader.h
	loader/TextureLoader.cpp
	loader/FileLoader.h
	loader/FileLoader.cpp
	loader/image/DDS.h
	loader/image/DDSLoader.h
	loader/image/DDSLoader.cpp
	system/Logging.h
	system/Logging.cpp
	system/utils.h
	system/Input.h
	system/Input.cpp
	system/JobSystem.h
	system/JobSystem.cpp
	system/FileSystem.h
	system/FileSystem.cpp
	system/Dialogs.h
	system/Dialogs.cpp
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
	utils/JsonUtils.h
)

find_package(Vulkan REQUIRED)

add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE)
set(GLM_STATIC_LIBRARY_ENABLE, true)
set(GLFW_BUILD_DOCS false)
set(GLFW_BUILD_EXAMPLES false)
set(GLFW_BUILD_TESTS false)
set(GLFW_INSTALL false)

add_subdirectory(lib/glfw-3.3)
set(EXPORT_TARGETS ${EXPORT_TARGETS} glfw) # remove?

set(OPTICK_ENABLED false)
set(OPTICK_USE_VULKAN false)
set(ozz_build_tools false)
set(ozz_build_fbx false)
set(ozz_build_gltf false)
set(ozz_build_data false)
set(ozz_build_samples false)
set(ozz_build_howtos false)
set(ozz_build_tests false)
set(ozz_build_simd_ref false)
set(ozz_build_msvc_rt_dll false)
set(ozz_build_postfix false)

set(OPTICK_BUILD_CONSOLE_SAMPLE, false)

add_subdirectory(lib/glm)
add_subdirectory(lib/SPIRV-Cross)
add_subdirectory(lib/optick)
add_subdirectory(lib/ozz-animation)

add_library(vk_engine ${ENGINE_SOURCES})
target_compile_definitions(vk_engine PUBLIC VK_USE_PLATFORM_WIN32_KHR)
target_include_directories(vk_engine PUBLIC ./ lib)
target_include_directories(vk_engine PUBLIC Vulkan::Vulkan)
target_include_directories(vk_engine PUBLIC ozz_animation)
target_link_libraries(vk_engine glfw)
target_link_libraries(vk_engine Vulkan::Vulkan)
target_link_libraries(vk_engine spirv-cross-core)
target_link_libraries(vk_engine spirv-cross-glsl)
target_link_libraries(vk_engine spirv-cross-cpp)
target_link_libraries(vk_engine spirv-cross-reflect)
target_link_libraries(vk_engine OptickCore)
target_link_libraries(vk_engine ozz_animation)
# dll .lib files
target_link_libraries(vk_engine ${CMAKE_CURRENT_SOURCE_DIR}/lib/static/dxc/dxcompiler.lib)
target_link_libraries(vk_engine ${CMAKE_CURRENT_SOURCE_DIR}/lib/static/physx/PhysX_64.lib)
target_link_libraries(vk_engine ${CMAKE_CURRENT_SOURCE_DIR}/lib/static/physx/PhysXCommon_64.lib)
target_link_libraries(vk_engine ${CMAKE_CURRENT_SOURCE_DIR}/lib/static/physx/PhysXFoundation_64.lib)
target_link_libraries(vk_engine ${CMAKE_CURRENT_SOURCE_DIR}/lib/static/physx/PhysXCooking_64.lib)

# static .lib files
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	target_link_libraries(vk_engine ${CMAKE_CURRENT_SOURCE_DIR}/lib/static/physx/PhysXPvdSDK_static_debug_64.lib)
	target_link_libraries(vk_engine ${CMAKE_CURRENT_SOURCE_DIR}/lib/static/physx/PhysXExtensions_static_debug_64.lib)
	target_link_libraries(vk_engine ${CMAKE_CURRENT_SOURCE_DIR}/lib/static/physx/PhysXVehicle_static_debug_64.lib)
	target_link_libraries(vk_engine ${CMAKE_CURRENT_SOURCE_DIR}/lib/static/physx/PhysXCharacterKinematic_static_debug_64.lib)
	
else (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	target_link_libraries(vk_engine ${CMAKE_CURRENT_SOURCE_DIR}/lib/static/physx/PhysXPvdSDK_static_64.lib)
	target_link_libraries(vk_engine ${CMAKE_CURRENT_SOURCE_DIR}/lib/static/physx/PhysXExtensions_static_64.lib)
	target_link_libraries(vk_engine ${CMAKE_CURRENT_SOURCE_DIR}/lib/static/physx/PhysXVehicle_static_64.lib)
	target_link_libraries(vk_engine ${CMAKE_CURRENT_SOURCE_DIR}/lib/static/physx/PhysXCharacterKinematic_static_64.lib)
endif (${CMAKE_BUILD_TYPE} STREQUAL "Debug")

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

if (VK_ENGINE_BUILD_TESTS)
	add_executable(engine_tests ${TEST_SOURCES})
	target_link_libraries(engine_tests vk_engine)
endif()