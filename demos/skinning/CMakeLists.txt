cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 17)

set(GAME_SOURCES
	main.cpp
	Skinning.h
	Skinning.cpp
	camera/ViewerCamera.h
	camera/ViewerCamera.cpp
)
 
include_directories(../../engine ../../engine/lib .)
add_executable(skinning ${GAME_SOURCES})
target_link_libraries(skinning vk_engine)
target_include_directories(skinning PRIVATE vk_engine)
