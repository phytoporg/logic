cmake_minimum_required (VERSION 3.8 FATAL_ERROR)

include_directories(${PROJECT_SOURCE_DIR}/src/lib)
target_link_libraries(${TARGETNAME}
    logik
    ${Vulkan_LIBRARY}
    glfw)
