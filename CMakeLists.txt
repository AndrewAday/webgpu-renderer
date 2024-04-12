include(FetchContent)

cmake_minimum_required(VERSION 3.0...3.25)
project(
    WebGPU-Renderer # name of the project, which will also be the name of the visual studio solution if you use it
    VERSION 0.1.0 # any version number
    LANGUAGES CXX C # programming languages used by the project
)

set(
    CORE 
    core/log.h core/log.c
)

set(
    EXAMPLES
    examples/example.h
    examples/basic.cpp
    examples/obj.cpp
)

add_executable(${CMAKE_PROJECT_NAME} 
    main.cpp
    common.h
    runner.h runner.cpp
    implementations.cpp
    memory.h memory.cpp
    context.h context.cpp
    shapes.h shapes.cpp
    entity.h entity.cpp
    shaders.h
    ${CORE}
    ${EXAMPLES}
)

# Defines
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
    RESOURCE_DIR="./resources"
    GLM_FORCE_DEPTH_ZERO_TO_ONE # glm force depth range to 0-1
    GLM_ENABLE_EXPERIMENTAL
    LOG_USE_COLOR
)


# Recommended extras
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    CXX_STANDARD 11
    CXX_EXTENSIONS OFF
    COMPILE_WARNING_AS_ERROR ON
    VS_DEBUGGER_ENVIRONMENT "DAWN_DEBUG_BREAK_ON_ERROR=1"
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
if (NOT EMSCRIPTEN)
    add_subdirectory(vendor/glfw)
endif()
add_subdirectory(vendor/glfw3webgpu)

# force use webgpu
set(WEBGPU_BACKEND "WGPU" CACHE STRING "Backend implementation of WebGPU. Possible values are WGPU and DAWN (it does not matter when using emcmake)")

# Ignore a warning that GLM requires to bypass
if (MSVC)
	# Disable warning C4201: nonstandard extension used: nameless struct/union
	target_compile_options(${CMAKE_PROJECT_NAME} PUBLIC /wd4201)
    # Set the startup project to be the actual project instead of ALL_BUILD 
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${CMAKE_PROJECT_NAME})

endif (MSVC)

# print which backend is being used
message(STATUS "Using WebGPU backend: ${WEBGPU_BACKEND}")

# TODO: host our own distribution on CCRMA
FetchContent_Declare(
  webgpu
  GIT_REPOSITORY https://github.com/eliemichel/WebGPU-distribution
  GIT_TAG        main
)
FetchContent_MakeAvailable(webgpu)

# linking
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE webgpu glfw glfw3webgpu)


# add vendor to include path
target_include_directories(${CMAKE_PROJECT_NAME} 
    PRIVATE .
    PRIVATE vendor
)

target_copy_webgpu_binaries(${CMAKE_PROJECT_NAME})

# emscripten specific options
if (EMSCRIPTEN)
    # know to rebuild when assets/ directory changes
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets)

    # Make sure to re-link when the shell file changes
    # set(SHELL_FILE shell_minimal.html)
    # set_property(
    #     TARGET ${CMAKE_PROJECT_NAME}
    #     PROPERTY LINK_DEPENDS
    #     "${CMAKE_CURRENT_SOURCE_DIR}/${SHELL_FILE}"
    # )


    # Add Emscripten-specific link options
    target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
        -sUSE_GLFW=3 # Use Emscripten-provided GLFW
        -sUSE_WEBGPU # Handle WebGPU symbols
        -sASYNCIFY # Required for emscripten_sleep in the async wgpu adapter and device request
        -sALLOW_MEMORY_GROWTH # allow memory to grow dynamically

        # uncomment to include local files in the build
        # https://emscripten.org/docs/porting/files/packaging_files.html
        # https://emscripten.org/docs/getting_started/Tutorial.html#tutorial-files
        # To include files outside of the current working directory
        # use the `--preload-file srcpath@dstpath` syntax to explicitly 
        # specify the target location
        --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/assets@./assets

        # Can specify custom html template
        # https://github.com/emscripten-core/emscripten/blob/main/src/shell_minimal.html
        # --shell-file "${CMAKE_CURRENT_SOURCE_DIR}/${SHELL_FILE}"
    )
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES SUFFIX ".html")

endif()