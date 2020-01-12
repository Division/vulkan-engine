cmake_minimum_required(VERSION 3.14)

set(GAME_SOURCES
	../game/objects/FollowCamera.h
	../game/objects/FollowCamera.cpp
	../game/objects/PlayerController.h
	../game/objects/PlayerController.cpp
	level/Level.h
	level/Level.cpp
	main.cpp
	Game.h
	Game.cpp
)
 
include_directories(../engine ../engine/lib)
add_executable(game_ecs ${GAME_SOURCES})
target_link_libraries(game_ecs vk_engine)
target_include_directories(game_ecs PRIVATE vk_engine)
