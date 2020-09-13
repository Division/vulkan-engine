cmake_minimum_required(VERSION 3.14)

project(FBXExporter)

set(CMAKE_CXX_STANDARD 17)

set(ENGINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../engine/)

set(SOURCE_LIST
	FBXExporterMain.cpp
	SceneExporter.h
	SceneExporter.cpp
	SceneExporterUtils.h
	SceneExporterUtils.cpp
	exporters/MeshExporter.h
	exporters/MeshExporter.cpp
	${ENGINE_DIR}utils/Math.h
	${ENGINE_DIR}utils/Math.cpp
	${ENGINE_DIR}utils/StringUtils.h
	${ENGINE_DIR}utils/StringUtils.cpp
	${ENGINE_DIR}lib/murmurhash/MurmurHash3.h
	${ENGINE_DIR}lib/murmurhash/MurmurHash3.cpp
)

add_subdirectory(${ENGINE_DIR}lib/glm out)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib ${ENGINE_DIR}lib)
add_executable(FBXExporter ${SOURCE_LIST}) 
target_link_libraries(FBXExporter ${CMAKE_CURRENT_SOURCE_DIR}/lib/static/libfbxsdk.lib)
target_link_libraries(FBXExporter ${CMAKE_CURRENT_SOURCE_DIR}/lib/static/zlib-mt.lib)

#target_link_libraries(FBXExporter ${ENGINE_DIR}lib/static/physx/PhysX_64.lib)
#target_link_libraries(FBXExporter ${ENGINE_DIR}lib/static/physx/PhysXCommon_64.lib)
target_link_libraries(FBXExporter ${ENGINE_DIR}lib/static/physx/PhysXFoundation_64.lib)
target_link_libraries(FBXExporter ${ENGINE_DIR}lib/static/physx/PhysXCooking_64.lib)
# static .lib files
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	target_link_libraries(FBXExporter ${ENGINE_DIR}lib/static/physx/PhysXExtensions_static_debug_64.lib)
else (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	target_link_libraries(FBXExporter ${ENGINE_DIR}lib/static/physx/PhysXExtensions_static_64.lib)
endif (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
