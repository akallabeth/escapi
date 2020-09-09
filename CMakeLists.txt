cmake_minimum_required(VERSION 3.4)

project(escapi VERSION 1.0.0.0 LANGUAGES C CXX)

set(CMAKE_COLOR_MAKEFILE ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(WITH_EXAMPLES "Build examples" ON)

include_directories(common)
add_subdirectory(common)
add_subdirectory(escapi_dll)
if (WITH_EXAMPLES)
    add_subdirectory(simplest)
endif()
