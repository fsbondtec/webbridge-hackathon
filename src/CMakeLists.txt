find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(httplib REQUIRED)
find_package(portable-file-dialogs REQUIRED)

FetchContent_Declare(
	webview
	GIT_REPOSITORY https://github.com/webview/webview
	GIT_TAG 0.12.0)
FetchContent_MakeAvailable(webview)

if(TARGET webview_core_shared)
	set_target_properties(webview_core_shared PROPERTIES FOLDER "CMakePredefinedTargets")
endif()
if(TARGET webview_core_static)
	set_target_properties(webview_core_static PROPERTIES FOLDER "CMakePredefinedTargets")
endif()

project(webbridge_hackathon LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(webbridge_hackathon_SOURCES
	main.cpp
	TestObject.cpp
	MyObject.cpp
	ResourceServer.h
	webbridge/impl/error_handler.cpp
)

set(webbridge_hackathon_WebBridge_SOURCES
	webbridge/impl/binding_helpers.h
	webbridge/impl/concepts.h
	webbridge/impl/error_handler.h
	webbridge/impl/error_handler.cpp
	webbridge/impl/event_impl.h
	webbridge/impl/object_registry.h
	webbridge/impl/property_impl.h
	webbridge/impl/type_registration.h
	webbridge/impl/type_registration.cpp

	webbridge/error.h
	webbridge/object.h
)

set(webbridge_hackathon_TO_REGISTRATION_SOURCES
	TestObject.h
	MyObject.h
)

add_executable(webbridge_hackathon WIN32
	${webbridge_hackathon_SOURCES}
	${webbridge_hackathon_WebBridge_SOURCES}
	${webbridge_hackathon_TO_REGISTRATION_SOURCES}
)

target_compile_features(webbridge_hackathon PRIVATE cxx_std_17)

target_link_libraries(webbridge_hackathon PRIVATE
	webview::core
	fmt::fmt
	nlohmann_json::nlohmann_json
	httplib::httplib
	portable-file-dialogs::portable-file-dialogs
	frontend_resources
)

webbridge_generate(
	TARGET webbridge_hackathon
	AUTO
	LANGUAGE cpp
)

set_target_properties(webbridge_hackathon PROPERTIES FOLDER src)
set_target_properties(webbridge_hackathon PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

target_compile_options(webbridge_hackathon PRIVATE /W3)

source_group("generated Files" REGULAR_EXPRESSION ".*_registration\.h$")
source_group(
	TREE "${CMAKE_CURRENT_SOURCE_DIR}"
	FILES
		${webbridge_hackathon_SOURCES}
		${webbridge_hackathon_WebBridge_SOURCES}
		${webbridge_hackathon_TO_REGISTRATION_SOURCES}
)

add_custom_command(TARGET webbridge_hackathon POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin

	COMMAND ${CMAKE_COMMAND} -E
	$<$<CONFIG:Debug>:echo> $<$<CONFIG:Debug>:"skip command in Debug mode: ">
	copy_directory_if_different $<TARGET_FILE_DIR:webbridge_hackathon> "${CMAKE_SOURCE_DIR}/bin"
)
