cmake_minimum_required(VERSION 3.14)

project(AssetManager)

set(CMAKE_CXX_STANDARD 17)

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
	utils/Dialogs.h
	utils/Dialogs.cpp
)

include_directories(../../engine ../../engine/lib .)
add_executable(ProjectManager ${SOURCE_LIST}) 
target_link_libraries(ProjectManager vk_engine)
target_include_directories(ProjectManager PRIVATE vk_engine)