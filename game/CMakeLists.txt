cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 17)

set(GAME_SOURCES
	main.cpp
	Game.h
	Game.cpp
	camera/ViewerCamera.h
	camera/ViewerCamera.cpp
	entities/vehicle/VehicleUtils.h
	entities/vehicle/VehicleUtils.cpp
	entities/vehicle/SceneQuery.h
	entities/vehicle/SceneQuery.cpp
	systems/ControlsSystem.h
	systems/ControlsSystem.cpp
	components/PlayerInput.h
	gameplay/Gameplay.h
	gameplay/Gameplay.cpp
	gameplay/GameplayUtils.h
	gameplay/GameplayUtils.cpp
	gameplay/components/Sphere.h
)
 
include_directories(../engine ../engine/lib .)
add_executable(game ${GAME_SOURCES})
target_link_libraries(game vk_engine)
target_include_directories(game PRIVATE vk_engine)
