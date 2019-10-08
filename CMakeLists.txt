cmake_minimum_required(VERSION 3.14)

include_directories(engine engine/lib)

set(
    CMAKE_RUNTIME_OUTPUT_DIRECTORY
    ${CMAKE_HOME_DIRECTORY}/bin
    )

add_subdirectory(engine)
add_subdirectory(game)
add_subdirectory(tools/ShaderCompiler)

