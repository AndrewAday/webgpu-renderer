include(FetchContent)

cmake_minimum_required(VERSION 3.0...3.25)
project(
    WebGPU-Renderer # name of the project, which will also be the name of the visual studio solution if you use it
    VERSION 0.1.0 # any version number
    LANGUAGES CXX C # programming languages used by the project
)

add_executable(${CMAKE_PROJECT_NAME} main.cpp)

# Recommended extras
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    CXX_STANDARD 11
    CXX_EXTENSIONS OFF
    COMPILE_WARNING_AS_ERROR ON
)

# increasing warning levels
if (MSVC)
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()

if(XCODE)
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
        XCODE_GENERATE_SCHEME ON
        XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal")
endif()

# vendor dependencies
add_subdirectory(vendor/glfw)
add_subdirectory(vendor/glfw3webgpu)

if (MSVC)
    # Dawn doesn't build on windows, use webgpu-native precompiled binaries
    set(WEBGPU_BACKEND "WGPU" CACHE STRING "Backend implementation of WebGPU. Possible values are WGPU and DAWN (it does not matter when using emcmake)")
else()
    # default to DAWN for source code and better error msgs
    set(WEBGPU_BACKEND "DAWN" CACHE STRING "Backend implementation of WebGPU. Possible values are WGPU and DAWN (it does not matter when using emcmake)")
endif()

FetchContent_Declare(
  webgpu
  GIT_REPOSITORY https://github.com/eliemichel/WebGPU-distribution
  GIT_TAG        main
)
FetchContent_MakeAvailable(webgpu)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE glfw webgpu glfw3webgpu)

# add vendor to include path
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE vendor)

target_copy_webgpu_binaries(${CMAKE_PROJECT_NAME})