cmake_minimum_required(VERSION 3.14)

project(AssetManager)

set(CMAKE_CXX_STANDARD 17)

set(ENGINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../engine/)
set(FBX_EXPORTER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../FBXExporter/)
set(OZZ_ANIMATION_DIR ${ENGINE_DIR}lib/ozz-animation/)
set(OZZ_ANIMATION_SRC_DIR ${OZZ_ANIMATION_DIR}src/)

set(SOURCE_LIST
	ProjectManagerMain.cpp
	UI/AssetTree.h
	UI/AssetTree.cpp
	AssetCache/AssetCache.h
	AssetCache/AssetCache.cpp
	AssetCache/AssetTypes.h
	AssetCache/AssetTypes.cpp
	AssetExport/ExportBase.h
	AssetExport/TextureExport.h
	AssetExport/TextureExport.cpp
	AssetExport/CopyExport.h
	AssetExport/CopyExport.cpp
	AssetExport/FBXExport.h
	AssetExport/FBXExport.cpp
	utils/Dialogs.h
	utils/Dialogs.cpp
	${FBX_EXPORTER_DIR}SceneExporter.h
	${FBX_EXPORTER_DIR}SceneExporter.cpp
	${FBX_EXPORTER_DIR}SceneExporterUtils.h
	${FBX_EXPORTER_DIR}SceneExporterUtils.cpp
	${FBX_EXPORTER_DIR}FBXExporterSettings.h
	${FBX_EXPORTER_DIR}exporters/FBXUtils.h
	${FBX_EXPORTER_DIR}exporters/FBXUtils.cpp
	${FBX_EXPORTER_DIR}exporters/MeshExporter.h
	${FBX_EXPORTER_DIR}exporters/MeshExporter.cpp
	${OZZ_ANIMATION_SRC_DIR}animation/offline/fbx/fbx.cc
	${OZZ_ANIMATION_SRC_DIR}animation/offline/fbx/fbx_animation.cc
	${OZZ_ANIMATION_SRC_DIR}animation/offline/fbx/fbx_skeleton.cc
)

include_directories(../../engine ../../engine/lib ${FBX_EXPORTER_DIR} ${FBX_EXPORTER_DIR}lib .)
add_executable(ProjectManager ${SOURCE_LIST}) 
target_link_libraries(ProjectManager vk_engine)
target_link_libraries(ProjectManager ozz_animation_offline)

target_include_directories(ProjectManager PRIVATE vk_engine)
 
target_link_libraries(ProjectManager ${FBX_EXPORTER_DIR}lib/static/libfbxsdk.lib)
target_link_libraries(ProjectManager ${FBX_EXPORTER_DIR}lib/static/zlib-mt.lib)
target_link_libraries(ProjectManager ${FBX_EXPORTER_DIR}lib/static/libxml2-mt.lib)
add_definitions(-DFBXSDK_SHARED) # dynamically linking FBXSDK needs this

target_link_libraries(ProjectManager ${ENGINE_DIR}lib/static/physx/PhysXFoundation_64.lib)
target_link_libraries(ProjectManager ${ENGINE_DIR}lib/static/physx/PhysXCooking_64.lib)
# static .lib files
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	target_link_libraries(ProjectManager ${ENGINE_DIR}lib/static/physx/PhysXExtensions_static_debug_64.lib)
else (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	target_link_libraries(ProjectManager ${ENGINE_DIR}lib/static/physx/PhysXExtensions_static_64.lib)
endif (${CMAKE_BUILD_TYPE} STREQUAL "Debug")