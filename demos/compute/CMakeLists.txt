#cmake_minimum_required(VERSION 3.14)

set(GAME_SOURCES
	main.cpp
	Compute.h
	Compute.cpp
	camera/ViewerCamera.h
	camera/ViewerCamera.cpp
 )

 # .g.c files will be generated from these
 set(RPSL
	"${CMAKE_CURRENT_SOURCE_DIR}/rps/test_triangle.rpsl"
	"${CMAKE_CURRENT_SOURCE_DIR}/rps/test_asd.rpsl"
 )
 
set(RESOURCES_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../bin/resources/compute/)
set(SHADER_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../bin/resources/compute/shaders/)

include_directories(.)
add_executable(compute ${GAME_SOURCES})

include("Modules")
include_module(compute blur ${RESOURCES_DIRECTORY})
include_rpsl(compute "${RPSL}")

target_link_libraries(compute vk_engine)
target_include_directories(compute PRIVATE vk_engine)

