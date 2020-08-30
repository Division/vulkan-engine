cmake_minimum_required(VERSION 3.14)

include_directories(engine engine/lib)

set(
    CMAKE_RUNTIME_OUTPUT_DIRECTORY
   ${CMAKE_HOME_DIRECTORY}/bin
)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")


add_subdirectory(engine)
add_subdirectory(game)
add_subdirectory(game_ecs)
add_subdirectory(model_viewer)
add_subdirectory(tools/ShaderCompiler) 
add_subdirectory(tools/FBXExporter) 

