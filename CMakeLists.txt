cmake_minimum_required (VERSION 3.8 FATAL_ERROR)

project ("Logik")

set (MyProject_VERSION_MAJOR 0)
set (MyProject_VERSION_MINOR 0)
set (MyProject_VERSION_PATCH 1)

set (CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/external/cmake)
message("MODULE PATH = ${CMAKE_MODULE_PATH}")

find_package(Vulkan REQUIRED FATAL_ERROR)
find_package(GLFW REQUIRED FATAL_ERROR)

add_subdirectory(src)

