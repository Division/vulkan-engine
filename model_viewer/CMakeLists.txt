cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_STANDARD 17)

set(SOURCES
	main.cpp
	ModelViewer.h
	ModelViewer.cpp
	camera/ViewerCamera.h
	camera/ViewerCamera.cpp
	GUI/ModelViewerGUI.h
	GUI/ModelViewerGUI.cpp
)
 
include_directories(../engine ../engine/lib)
add_executable(model_viewer ${SOURCES})
target_link_libraries(model_viewer vk_engine)
target_include_directories(model_viewer PRIVATE vk_engine)
