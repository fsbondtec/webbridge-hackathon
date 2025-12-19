cmake_minimum_required(VERSION 3.26)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

project(
	webbridge_hackathon
	VERSION 1.0.0
	LANGUAGES CXX C
)
add_definitions(-D_WIN32_WINNT=0x0A00)

include(webbridge)
include(FetchContent)

find_package(Python REQUIRED COMPONENTS Interpreter)

FetchContent_Declare(
	cmrc
	GIT_REPOSITORY https://github.com/vector-of-bool/cmrc.git
	GIT_TAG master
)
FetchContent_MakeAvailable(cmrc)

add_subdirectory(frontend)
add_subdirectory(src)
set_directory_properties(PROPERTIES VS_STARTUP_PROJECT webbridge_hackathon)
