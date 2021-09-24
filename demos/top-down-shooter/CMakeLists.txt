cmake_minimum_required(VERSION 3.14)

set(GAME_SOURCES
	main.cpp
	TopDownShooter.h
	TopDownShooter.cpp
	camera/ViewerCamera.h
	camera/ViewerCamera.cpp
	controller/Controller.h
	controller/Controller.cpp
	controller/Player.h
	controller/Player.cpp
	projectile/Projectile.h
	projectile/Projectile.cpp
)
 
include_directories(../../engine ../../engine/lib .)
add_executable(top-down-shooter ${GAME_SOURCES})
target_link_libraries(top-down-shooter vk_engine)
target_include_directories(top-down-shooter PRIVATE vk_engine)
