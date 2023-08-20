function(include_rpsl targetName rpsl)
	set(rpsl_hlslc_exe "${CMAKE_CURRENT_LIST_DIR}/../../bin/rps/rps-hlslc.exe")

	foreach(src_file ${rpsl})
		set(file_g_c "${src_file}.g.c")
		get_filename_component(src_directory ${src_file} DIRECTORY)
		get_filename_component(src_no_ext ${src_file} NAME_WE)

		add_custom_command(
			OUTPUT ${file_g_c}
			COMMAND ${rpsl_hlslc_exe} ${src_file} -O3 -od ${src_directory}
			COMMAND ${CMAKE_COMMAND} -E remove -f "${src_directory}/${src_no_ext}.tmp.rps.ll"
			DEPENDS ${src_file}
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		)

		target_sources(${targetName} PRIVATE ${file_g_c})
	endforeach()

endfunction()

function(include_module targetName moduleName resourceDir)
    list(LENGTH ARGN NUM_EXTRA_ARGS)

    if(NUM_EXTRA_ARGS)
        list(GET ARGN 0 shaderDir)
    else()
        set(shaderDir ${resourceDir})
    endif()

	set(modules_dir "${CMAKE_CURRENT_LIST_DIR}/../modules")
	add_subdirectory(../../engine/modules/${moduleName} "${CMAKE_CURRENT_BINARY_DIR}/${moduleName}")
	target_sources(${targetName} PRIVATE ${MODULE_SOURCES})

	foreach(src_file ${MODULE_HLSL})
		get_filename_component(filename ${src_file} NAME)
		set(output_file "${shaderDir}/${filename}")

		add_custom_command(
			OUTPUT ${output_file}
			COMMAND ${CMAKE_COMMAND} -E copy ${src_file} ${output_file}
			DEPENDS ${src_file}
			COMMENT "Copying ${filename} to ${dest_dir}"
		)

		# Add to list of outputs
		list(APPEND copiedFiles ${output_file})
	endforeach()

	include_rpsl(${targetName} "${MODULE_RPSL}")

	add_custom_target("${moduleName}_copy_files" ALL
		DEPENDS ${copiedFiles}
	)

	add_dependencies(${targetName} "${moduleName}_copy_files")
 
endfunction()


