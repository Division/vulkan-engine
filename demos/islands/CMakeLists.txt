set(GAME_SOURCES
	main.cpp
	islands.h
	islands.cpp
	camera/ViewerCamera.h
	camera/ViewerCamera.cpp
	generation/LandMeshGenerator.h
	generation/LandMeshGenerator.cpp
	resources/Heightmap.h
)

include_directories(../../engine ../../engine/lib .)
add_executable(islands ${GAME_SOURCES})
target_link_libraries(islands vk_engine)
target_include_directories(islands PRIVATE vk_engine)

set_target_properties(islands PROPERTIES JOB_POOL_COMPILE one_jobs)