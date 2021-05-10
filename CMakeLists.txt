cmake_policy(SET CMP0091 NEW)
cmake_minimum_required(VERSION 3.15)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

option(VK_ENGINE_AUTO_SETUP_BUILD "Auto setup build" ON)
option(VK_ENGINE_BUILD_DEMOS "Build demos" ON)
option(VK_ENGINE_BUILD_TOOLS "Build tools" ON)

if (VK_ENGINE_AUTO_SETUP_BUILD)
	include("engine/cmake/Configuration.cmake")
	vk_engine_set_build_params()
endif()

add_subdirectory(engine)

if (VK_ENGINE_BUILD_DEMOS)
	add_subdirectory(demos/action-puzzle)
	add_subdirectory(demos/skinning)
endif()

if (VK_ENGINE_BUILD_TOOLS)
	add_subdirectory(tools/ShaderCompiler)
	add_subdirectory(tools/FBXExporter)
	add_subdirectory(tools/ProjectManager)
endif()