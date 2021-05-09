cmake_policy(SET CMP0091 NEW)
cmake_minimum_required(VERSION 3.15)

include_directories(engine engine/lib)

set(
    CMAKE_RUNTIME_OUTPUT_DIRECTORY
   ${CMAKE_CURRENT_SOURCE_DIR}/bin
)

#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

message(STATUS "CMAKE_C_FLAGS_DEBUG: ${CMAKE_C_FLAGS_DEBUG}")
message(STATUS "CMAKE_C_FLAGS_RELEASE: ${CMAKE_C_FLAGS_RELEASE}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Ob0 /Od /RTC1 /MTd")
set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Ob2 /DNDEBUG /Zi /MT")
set(CMAKE_C_FLAGS_DEBUG "/MTd /Zi /Ob0 /Od /RTC1")
set(CMAKE_C_FLAGS_RELEASE "/MT /O2 /Ob2 /DNDEBUG")

set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
 
message(STATUS "CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")

set(GLFW_BUILD_DOCS false)
set(GLFW_BUILD_EXAMPLES false)
set(GLFW_BUILD_TESTS false)
set(GLFW_INSTALL false)

add_subdirectory(engine)
add_subdirectory(demos/action-puzzle)
add_subdirectory(demos/skinning)
add_subdirectory(tools/ShaderCompiler)
add_subdirectory(tools/FBXExporter)
add_subdirectory(tools/ProjectManager)
