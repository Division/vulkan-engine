#cmake_minimum_required(VERSION 3.14)

set(GAME_SOURCES
	main.cpp
	ParticleSystem.h
	ParticleSystem.cpp
	camera/ViewerCamera.h
	camera/ViewerCamera.cpp
)
 
include_directories(../../engine ../../engine/lib .)
add_executable(particle_system ${GAME_SOURCES})
target_link_libraries(particle_system vk_engine)
target_include_directories(particle_system PRIVATE vk_engine)
