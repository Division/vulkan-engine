cmake_minimum_required(VERSION 3.14)

project(ShaderCompiler)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_LIST
	ShaderCompilerMain.cpp
)

include_directories(../../engine ../../engine/lib)
add_executable(ShaderCompiler ${SOURCE_LIST}) 
target_link_libraries(ShaderCompiler vk_engine)