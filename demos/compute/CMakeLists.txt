#cmake_minimum_required(VERSION 3.14)

set(GAME_SOURCES
	main.cpp
	Compute.h
	Compute.cpp
	camera/ViewerCamera.h
	camera/ViewerCamera.cpp
)
 
include_directories(../../engine ../../engine/lib .)
add_executable(compute ${GAME_SOURCES})
target_link_libraries(compute vk_engine)
target_include_directories(compute PRIVATE vk_engine)
