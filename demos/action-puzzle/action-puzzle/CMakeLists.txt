cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 17)

set(GAME_SOURCES
	main.cpp
	ActionPuzzle.h
	ActionPuzzle.cpp
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
 
include_directories(../../engine ../../engine/lib .)
add_executable(action_puzzle ${GAME_SOURCES})
target_link_libraries(action_puzzle vk_engine)
target_include_directories(action_puzzle PRIVATE vk_engine)
