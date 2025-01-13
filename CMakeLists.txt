cmake_minimum_required(VERSION 3.31)

project(game VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Vulkan REQUIRED)

add_executable(${CMAKE_PROJECT_NAME})
add_subdirectory(src)
add_subdirectory(ext/glm)
add_subdirectory(ext/glfw)

set(SHADERC_SKIP_TESTS ON CACHE BOOL "Skip building tests" FORCE)
set(SHADERC_SKIP_INSTALL ON CACHE BOOL "Skip building tests" FORCE)
set(SHADERC_SKIP_EXAMPLES ON CACHE BOOL "Skip building tests" FORCE)
set(EFFCEE_BUILD_TESTING OFF CACHE BOOL "Enable testing for Effcee" FORCE)
set(GLSLANG_ENABLE_INSTALL OFF CACHE BOOL "Enable glslang installation" FORCE)
set(GLSLANG_TESTS OFF CACHE BOOL "Enable glslang testing" FORCE)
add_subdirectory(ext/shaderc)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE src)
target_link_libraries(${CMAKE_PROJECT_NAME} glm::glm)
target_link_libraries(${CMAKE_PROJECT_NAME} glfw)
target_link_libraries(${CMAKE_PROJECT_NAME} Vulkan::Vulkan)
target_link_libraries(${CMAKE_PROJECT_NAME} shaderc)

