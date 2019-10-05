cmake_minimum_required(VERSION 3.9)

project(vk_game)

set(GAME_SOURCES
	main.cpp
	game/Game.h
	game/Game.cpp
	game/level/Level.h
	game/level/Level.cpp
	game/objects/FollowCamera.h
	game/objects/FollowCamera.cpp
	game/objects/PlayerController.h
	game/objects/PlayerController.cpp
)
 
include_directories(src src/lib)
add_subdirectory(src)
add_executable(game ${GAME_SOURCES})
target_link_libraries(game vk_engine)
target_include_directories(game PRIVATE vk_engine)