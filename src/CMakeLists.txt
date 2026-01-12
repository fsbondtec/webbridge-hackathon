find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(httplib REQUIRED)
find_package(portable-file-dialogs REQUIRED)

FetchContent_Declare(
	webview
	GIT_REPOSITORY https://github.com/webview/webview
	GIT_TAG 0.12.0)
FetchContent_MakeAvailable(webview)

project(webbridge_hackathon LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(webbridge_hackathon_SOURCES
	main.cpp
	TestObject.cpp
	TestObject1.cpp
	TestObject2.cpp
	TestObject3.cpp
	TestObject4.cpp
	TestObject5.cpp
	TestObject6.cpp
	TestObject7.cpp
	TestObject8.cpp
	TestObject9.cpp
	TestObject10.cpp
	TestObject11.cpp
	TestObject12.cpp
	TestObject13.cpp
	TestObject14.cpp
	TestObject15.cpp
	TestObject16.cpp
	TestObject17.cpp
	TestObject18.cpp
	TestObject19.cpp
	TestObject20.cpp
	TestObject21.cpp
	TestObject22.cpp
	TestObject23.cpp
	TestObject24.cpp
	TestObject25.cpp
	TestObject26.cpp
	TestObject27.cpp
	TestObject28.cpp
	TestObject29.cpp
	TestObject30.cpp
	TestObject31.cpp
	TestObject32.cpp
	TestObject33.cpp
	TestObject34.cpp
	TestObject35.cpp
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
	webbridge/impl/thread_pool.h
	webbridge/impl/thread_pool.cpp
	webbridge/impl/type_registration.h
	webbridge/impl/type_registration.cpp
	webbridge/error.h
	webbridge/object.h
)

set(webbridge_hackathon_TO_REGISTRATION_SOURCES
	TestObject.h
	TestObject1.h
	TestObject2.h
	TestObject3.h
	TestObject4.h
	TestObject5.h
	TestObject6.h
	TestObject7.h
	TestObject8.h
	TestObject9.h
	TestObject10.h
	TestObject11.h
	TestObject12.h
	TestObject13.h
	TestObject14.h
	TestObject15.h
	TestObject16.h
	TestObject17.h
	TestObject18.h
	TestObject19.h
	TestObject20.h
	TestObject21.h
	TestObject22.h
	TestObject23.h
	TestObject24.h
	TestObject25.h
	TestObject26.h
	TestObject27.h
	TestObject28.h
	TestObject29.h
	TestObject30.h
	TestObject31.h
	TestObject32.h
	TestObject33.h
	TestObject34.h
	TestObject35.h
	MyObject.h
)

add_executable(webbridge_hackathon WIN32
	${webbridge_hackathon_SOURCES}
	${webbridge_hackathon_WebBridge_SOURCES}
	${webbridge_hackathon_TO_REGISTRATION_SOURCES}
)

target_compile_features(webbridge_hackathon PRIVATE cxx_std_20)

target_link_libraries(webbridge_hackathon PRIVATE
	webview::core
	fmt::fmt
	nlohmann_json::nlohmann_json
	httplib::httplib
	portable-file-dialogs::portable-file-dialogs
	frontend_resources
)

target_precompile_headers(webbridge_hackathon PRIVATE pch.h)

webbridge_generate(
	TARGET webbridge_hackathon
	AUTO
	LANGUAGE cpp
)

target_compile_options(webbridge_hackathon PRIVATE /W3)

add_custom_command(TARGET webbridge_hackathon POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin

	COMMAND ${CMAKE_COMMAND} -E
	$<$<CONFIG:Debug>:echo> $<$<CONFIG:Debug>:"skip command in Debug mode: ">
	copy_directory_if_different $<TARGET_FILE_DIR:webbridge_hackathon> "${CMAKE_SOURCE_DIR}/bin"
)
