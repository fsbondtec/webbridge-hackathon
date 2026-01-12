# Helper function to parse discoverer output
function(_parse_discoverer_output discoverer_output out_var)
	set(result)
	if(discoverer_output)
		string(REPLACE "\n" ";" lines "${discoverer_output}")
		foreach(line ${lines})
			if(line)
				string(REPLACE "|" ";" parts "${line}")
				list(GET parts 0 file)
				list(GET parts 1 class_name)
				list(APPEND result "${file}|${class_name}")
			endif()
		endforeach()
	endif()
	set(${out_var} ${result} PARENT_SCOPE)
endfunction()

function(webbridge_generate)
	# Check Python availability
	if(NOT Python_EXECUTABLE)
		find_package(Python REQUIRED)
	endif()

	set(options AUTO)
	set(oneValueArgs TARGET OUTPUT_DIR LANGUAGE FILE CLASS_NAME)
	set(multiValueArgs FILES)
	cmake_parse_arguments(PARSE_ARGV 0 arg
		"${options}" "${oneValueArgs}" "${multiValueArgs}"
	)

	# Validate parameter combinations
	if(arg_FILE AND arg_FILES)
		message(FATAL_ERROR "webbridge_generate: Use either FILE or FILES, not both")
	endif()

	if(arg_FILE AND arg_AUTO)
		message(FATAL_ERROR "webbridge_generate: FILE cannot be used with AUTO")
	endif()

	if(arg_AUTO AND arg_FILES)
		message(FATAL_ERROR "webbridge_generate: Use either AUTO or FILES, not both")
	endif()

	if(arg_FILE AND NOT arg_CLASS_NAME)
		message(FATAL_ERROR "webbridge_generate: FILE requires CLASS_NAME")
	endif()

	if(arg_CLASS_NAME AND NOT arg_FILE)
		message(FATAL_ERROR "webbridge_generate: CLASS_NAME requires FILE")
	endif()

	if(NOT arg_AUTO AND NOT arg_FILES AND NOT arg_FILE)
		message(FATAL_ERROR "webbridge_generate: Either AUTO, FILES, or FILE must be specified")
	endif()

	# Default to CMAKE_CURRENT_BINARY_DIR if not specified
	if(NOT arg_OUTPUT_DIR)
		set(arg_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
	endif()

	# Default to cpp if not specified
	if(NOT arg_LANGUAGE)
		set(arg_LANGUAGE cpp)
	endif()

	# Collect files to process
	set(all_files)

	# Explicit FILE + CLASS_NAME mode (no discovery)
	if(arg_FILE)
		get_filename_component(abs_file "${arg_FILE}" ABSOLUTE)
		list(APPEND all_files "${abs_file}|${arg_CLASS_NAME}")
	# Collect auto-detected header files from target if AUTO flag is set
	elseif(arg_AUTO)
		set(header_files)
		get_target_property(target_sources ${arg_TARGET} SOURCES)
		if(target_sources)
			foreach(source ${target_sources})
				# Get absolute path
				get_filename_component(abs_source "${source}" ABSOLUTE)
				# Check if it's a header file
				if(abs_source MATCHES "\\.(h|hpp)$")
					list(APPEND header_files ${abs_source})
				endif()
			endforeach()
		endif()

		if(header_files)
			execute_process(
				COMMAND ${Python_EXECUTABLE}
				${CMAKE_SOURCE_DIR}/tools/discoverer.py
					${header_files}
				WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
				OUTPUT_VARIABLE discoverer_output
				OUTPUT_STRIP_TRAILING_WHITESPACE
				RESULT_VARIABLE result
			)

			if(result AND NOT result EQUAL 0)
				message(FATAL_ERROR "discoverer.py failed with exit code ${result}")
			endif()

			# Parse output using helper function
			_parse_discoverer_output("${discoverer_output}" all_files)
		endif()
	# Process explicit FILES - discover classes in these files
	elseif(arg_FILES)
		set(abs_files)
		foreach(file ${arg_FILES})
			get_filename_component(abs_file "${file}" ABSOLUTE)
			list(APPEND abs_files ${abs_file})
		endforeach()

		execute_process(
			COMMAND ${Python_EXECUTABLE}
			${CMAKE_SOURCE_DIR}/tools/discoverer.py
				${abs_files}
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			OUTPUT_VARIABLE discoverer_output
			OUTPUT_STRIP_TRAILING_WHITESPACE
			RESULT_VARIABLE result
		)

		if(result AND NOT result EQUAL 0)
			message(FATAL_ERROR "discoverer.py failed with exit code ${result}")
		endif()

		# Parse output using helper function
		_parse_discoverer_output("${discoverer_output}" all_files)
	endif()

	# Check if we have any files to process
	if(NOT all_files)
		if(arg_AUTO)
			message(WARNING "webbridge_generate: No files with webbridge::object classes found in target ${arg_TARGET}")
		else()
			message(WARNING "webbridge_generate: No files to process")
		endif()
		return()
	endif()

	# Process each file|classname pair
	foreach(pair ${all_files})
		# Split pair into file and class_name
		string(REPLACE "|" ";" parts "${pair}")
		list(GET parts 0 file)
		list(GET parts 1 class_name)

		# Determine output file, template and python args based on LANGUAGE
		if(arg_LANGUAGE STREQUAL "cpp")
			set(output_h_file "${arg_OUTPUT_DIR}/${class_name}_registration.h")
			set(output_cpp_file "${arg_OUTPUT_DIR}/${class_name}_registration.cpp")
			set(template_h_file "${CMAKE_SOURCE_DIR}/tools/templates/registration_header.h.j2")
			set(template_cpp_file "${CMAKE_SOURCE_DIR}/tools/templates/registration_impl.cpp.j2")
			set(python_out_arg --cpp_out)
		elseif(arg_LANGUAGE STREQUAL "ts-impl")
			set(output_file "${arg_OUTPUT_DIR}/${class_name}.ts")
			set(template_file "${CMAKE_SOURCE_DIR}/tools/templates/impl.ts.j2")
			set(python_out_arg --ts_impl_out)
		else()
			message(FATAL_ERROR "Invalid LANGUAGE: ${arg_LANGUAGE}. Must be 'cpp', 'ts', or 'ts-impl'")
		endif()

		message(STATUS "Generating ${arg_LANGUAGE} for ${file} -> ${class_name}")
		
		if(arg_LANGUAGE STREQUAL "cpp")
			# For C++, generate both .h and .cpp files
			add_custom_command(
				OUTPUT ${output_h_file} ${output_cpp_file}
				COMMAND ${Python_EXECUTABLE}
				ARGS
				${CMAKE_SOURCE_DIR}/tools/generate.py
				${file}
				--class-name ${class_name}
				${python_out_arg} ${arg_OUTPUT_DIR}
			DEPENDS
				${CMAKE_SOURCE_DIR}/tools/generate.py
					${CMAKE_SOURCE_DIR}/tools/parser.py
					${CMAKE_SOURCE_DIR}/tools/tstypes.py
					${template_h_file}
					${template_cpp_file}
					${file}
				COMMENT "Running webbridge registration on ${file} (${class_name}) for ${arg_LANGUAGE}"
				VERBATIM
			)

			# Add both .h and .cpp to target sources
			set_source_files_properties(${output_h_file} PROPERTIES GENERATED TRUE)
			set_source_files_properties(${output_cpp_file} PROPERTIES GENERATED TRUE)
			target_sources(${arg_TARGET} PRIVATE ${output_h_file} ${output_cpp_file})
			
			# Add generated header directory to include path
			get_filename_component(gen_header_dir ${output_h_file} DIRECTORY)
			target_include_directories(${arg_TARGET} PRIVATE ${gen_header_dir})
		else()
			# For TypeScript, generate single file
			add_custom_command(
				OUTPUT ${output_file}
				COMMAND ${Python_EXECUTABLE}
				ARGS
				${CMAKE_SOURCE_DIR}/tools/generate.py
				${file}
				--class-name ${class_name}
				${python_out_arg} ${arg_OUTPUT_DIR}
			DEPENDS
				${CMAKE_SOURCE_DIR}/tools/generate.py
					${CMAKE_SOURCE_DIR}/tools/parser.py
					${CMAKE_SOURCE_DIR}/tools/tstypes.py
					${template_file}
					${file}
				COMMENT "Running webbridge registration on ${file} (${class_name}) for ${arg_LANGUAGE}"
				VERBATIM
			)
		endif()
	endforeach()
endfunction()
