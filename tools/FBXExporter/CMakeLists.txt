cmake_minimum_required(VERSION 3.14)

project(FBXExporter)

set(CMAKE_CXX_STANDARD 17)

set(ENGINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../engine/)
set(OZZ_ANIMATION_DIR ${ENGINE_DIR}lib/ozz-animation/)
set(OZZ_ANIMATION_SRC_DIR ${OZZ_ANIMATION_DIR}src/)
set(OZZ_ANIMATION_INCLUDE_DIR ${OZZ_ANIMATION_DIR}include/)

set (OZZ_SOURCE_LIST
	${OZZ_ANIMATION_SRC_DIR}animation/offline/fbx/fbx.cc
	${OZZ_ANIMATION_SRC_DIR}animation/offline/fbx/fbx_animation.cc
	${OZZ_ANIMATION_SRC_DIR}animation/offline/fbx/fbx_skeleton.cc
	${OZZ_ANIMATION_SRC_DIR}animation/offline/additive_animation_builder.cc
	${OZZ_ANIMATION_SRC_DIR}animation/offline/animation_builder.cc
	${OZZ_ANIMATION_SRC_DIR}animation/offline/animation_optimizer.cc
	${OZZ_ANIMATION_SRC_DIR}animation/offline/decimate.h
	${OZZ_ANIMATION_SRC_DIR}animation/offline/raw_animation.cc
	${OZZ_ANIMATION_SRC_DIR}animation/offline/raw_animation_archive.cc
	${OZZ_ANIMATION_SRC_DIR}animation/offline/raw_animation_utils.cc
	${OZZ_ANIMATION_SRC_DIR}animation/offline/raw_skeleton.cc
	${OZZ_ANIMATION_SRC_DIR}animation/offline/raw_skeleton_archive.cc
	${OZZ_ANIMATION_SRC_DIR}animation/offline/raw_track.cc
	${OZZ_ANIMATION_SRC_DIR}animation/offline/skeleton_builder.cc
	${OZZ_ANIMATION_SRC_DIR}animation/offline/track_builder.cc
	${OZZ_ANIMATION_SRC_DIR}animation/offline/track_optimizer.cc

	${OZZ_ANIMATION_SRC_DIR}animation/runtime/animation.cc
	${OZZ_ANIMATION_SRC_DIR}animation/runtime/animation_keyframe.h
	${OZZ_ANIMATION_SRC_DIR}animation/runtime/animation_utils.cc
	${OZZ_ANIMATION_SRC_DIR}animation/runtime/blending_job.cc
	${OZZ_ANIMATION_SRC_DIR}animation/runtime/ik_aim_job.cc
	${OZZ_ANIMATION_SRC_DIR}animation/runtime/ik_two_bone_job.cc
	${OZZ_ANIMATION_SRC_DIR}animation/runtime/local_to_model_job.cc
	${OZZ_ANIMATION_SRC_DIR}animation/runtime/sampling_job.cc
	${OZZ_ANIMATION_SRC_DIR}animation/runtime/skeleton.cc
	${OZZ_ANIMATION_SRC_DIR}animation/runtime/skeleton_utils.cc
	${OZZ_ANIMATION_SRC_DIR}animation/runtime/track.cc
	${OZZ_ANIMATION_SRC_DIR}animation/runtime/track_sampling_job.cc
	${OZZ_ANIMATION_SRC_DIR}animation/runtime/track_triggering_job.cc

	${OZZ_ANIMATION_SRC_DIR}base/containers/string_archive.cc
	${OZZ_ANIMATION_SRC_DIR}base/io/archive.cc
	${OZZ_ANIMATION_SRC_DIR}base/io/stream.cc
	${OZZ_ANIMATION_SRC_DIR}base/maths/box.cc
	${OZZ_ANIMATION_SRC_DIR}base/maths/math_archive.cc
	${OZZ_ANIMATION_SRC_DIR}base/maths/simd_math.cc
	${OZZ_ANIMATION_SRC_DIR}base/maths/simd_math_archive.cc
	${OZZ_ANIMATION_SRC_DIR}base/maths/soa_math_archive.cc
	${OZZ_ANIMATION_SRC_DIR}base/memory/allocator.cc
	${OZZ_ANIMATION_SRC_DIR}base/log.cc
	${OZZ_ANIMATION_SRC_DIR}base/platform.cc
	
	${OZZ_ANIMATION_SRC_DIR}geometry/runtime/skinning_job.cc
	${OZZ_ANIMATION_SRC_DIR}options/options.cc
)

set(SOURCE_LIST
	FBXExporterMain.cpp
	SceneExporter.h
	SceneExporter.cpp
	SceneExporterUtils.h
	SceneExporterUtils.cpp
	exporters/MeshExporter.h
	exporters/MeshExporter.cpp
	exporters/FBXUtils.h
	exporters/FBXUtils.cpp
	${ENGINE_DIR}utils/Math.h
	${ENGINE_DIR}utils/Math.cpp
	${ENGINE_DIR}utils/StringUtils.h
	${ENGINE_DIR}utils/StringUtils.cpp
	${ENGINE_DIR}lib/murmurhash/MurmurHash3.h
	${ENGINE_DIR}lib/murmurhash/MurmurHash3.cpp
	${OZZ_SOURCE_LIST}
)

set(ozz_build_tools false)
set(ozz_build_fbx false)
set(ozz_build_gltf false)
set(ozz_build_data false)
set(ozz_build_samples false)
set(ozz_build_howtos false)
set(ozz_build_tests false)
set(ozz_build_simd_ref false)
set(ozz_build_msvc_rt_dll false)
set(ozz_build_postfix false)
add_subdirectory(${ENGINE_DIR}lib/glm out)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib ${ENGINE_DIR}lib ${OZZ_ANIMATION_INCLUDE_DIR} ${OZZ_ANIMATION_SRC_DIR})
add_executable(FBXExporter ${SOURCE_LIST}) 
target_link_libraries(FBXExporter ${CMAKE_CURRENT_SOURCE_DIR}/lib/static/libfbxsdk.lib)
target_link_libraries(FBXExporter ${CMAKE_CURRENT_SOURCE_DIR}/lib/static/zlib-mt.lib)
target_link_libraries(FBXExporter ${CMAKE_CURRENT_SOURCE_DIR}/lib/static/libxml2-mt.lib)
target_link_libraries(FBXExporter ozz_animation)
add_definitions(-DFBXSDK_SHARED) # dynamically linking FBXSDK needs this

target_link_libraries(FBXExporter ${ENGINE_DIR}lib/static/physx/PhysXFoundation_64.lib)
target_link_libraries(FBXExporter ${ENGINE_DIR}lib/static/physx/PhysXCooking_64.lib)
# static .lib files
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	target_link_libraries(FBXExporter ${ENGINE_DIR}lib/static/physx/PhysXExtensions_static_debug_64.lib)
else (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	target_link_libraries(FBXExporter ${ENGINE_DIR}lib/static/physx/PhysXExtensions_static_64.lib)
endif (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
