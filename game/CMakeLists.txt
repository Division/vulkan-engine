cmake_minimum_required(VERSION 3.14)

set(GAME_SOURCES
	main.cpp
	Game.h
	Game.cpp
	level/Level.h
	level/Level.cpp
	objects/FollowCamera.h
	objects/FollowCamera.cpp
	objects/PlayerController.h
	objects/PlayerController.cpp
)
 
include_directories(../engine ../engine/lib)
add_executable(game ${GAME_SOURCES})
target_link_libraries(game vk_engine)
target_include_directories(game PRIVATE vk_engine)
