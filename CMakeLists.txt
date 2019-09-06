cmake_minimum_required(VERSION 3.9)
project(vk_engine)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
#SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -M)

set(ENGINE_SOURCES 
	main.cpp
	src/lib/VulkanMemoryAllocator/vk_mem_alloc.h
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
	src/render/device/CommandBufferManager.h
	src/render/device/CommandBufferManager.cpp
	src/render/device/Device.cpp
	src/render/device/VulkanContext.h
	src/render/device/VulkanContext.cpp
	src/render/buffer/VulkanBuffer.h
	src/render/buffer/VulkanBuffer.cpp
	src/render/device/VulkanUploader.h
	src/render/device/VulkanUploader.cpp
	src/render/device/VulkanUtils.h
	src/render/device/VulkanUtils.cpp
	src/render/mesh/Mesh.h
	src/render/mesh/Mesh.cpp
	src/resources/ModelBundle.h
	src/resources/ModelBundle.cpp
	src/resources/HierarchyData.h
	src/resources/HierarchyData.cpp
	src/resources/SkinningData.h
	src/resources/SkinningData.cpp
	src/loader/ModelLoader.h
	src/loader/ModelLoader.cpp
	src/loader/ModelLoaderUtils.h
	src/loader/ModelLoaderUtils.cpp
	src/system/Logging.h
	src/system/Logging.cpp
	src/system/utils.h
	src/utils/Math.h
	src/utils/Math.cpp
	src/utils/CapsSet.h
	src/utils/Frustum.h
)

set(GAME_SOURCES
	game/Game.h
	game/Game.cpp
	game/GameUtils.h
	game/GameUtils.cpp
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

add_executable(vk_engine ${ENGINE_SOURCES} ${GAME_SOURCES})
target_compile_definitions(vk_engine PRIVATE VK_USE_PLATFORM_WIN32_KHR)
target_include_directories(vk_engine PRIVATE Vulkan::Vulkan)
target_link_libraries(vk_engine glfw)
target_link_libraries(vk_engine Vulkan::Vulkan)

add_executable(example_texture samples/25_texture_mapping.cpp src/lib/stb/stb_image.h)
target_compile_definitions(example_texture PRIVATE VK_USE_PLATFORM_WIN32_KHR)
target_include_directories(example_texture PRIVATE Vulkan::Vulkan)
target_link_libraries(example_texture glfw)
target_link_libraries(example_texture Vulkan::Vulkan)