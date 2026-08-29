cmake_minimum_required(VERSION 3.15)
project(VoltronMissionGalraCypher CXX)

# Require C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Disable raylib examples and games to speed up the build
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

# Use FetchContent to automatically download and configure Raylib
include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
)
FetchContent_MakeAvailable(raylib)

# Recursively find all C++ source files in the src/ directory
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

# Create the main executable target
add_executable(${PROJECT_NAME} ${SOURCES})

option(ENABLE_ADDRESS_SANITIZER "Enable AddressSanitizer for memory debugging" OFF)
option(ENABLE_UNDEFINED_SANITIZER "Enable UndefinedBehaviorSanitizer" OFF)
option(VOLTRON_WARNINGS_AS_ERRORS "Treat project warnings as errors" OFF)
option(VOLTRON_ENABLE_IPO "Enable interprocedural optimization in Release" ON)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive-)
    if(VOLTRON_WARNINGS_AS_ERRORS)
        target_compile_options(${PROJECT_NAME} PRIVATE /WX)
    endif()
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wformat=2
        -Wnull-dereference
        -Wnon-virtual-dtor
    )
    if(VOLTRON_WARNINGS_AS_ERRORS)
        target_compile_options(${PROJECT_NAME} PRIVATE -Werror)
    endif()
endif()

if(ENABLE_ADDRESS_SANITIZER)
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /fsanitize=address /Zi)
        target_link_options(${PROJECT_NAME} PRIVATE /INCREMENTAL:NO)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
    endif()
endif()

if(ENABLE_UNDEFINED_SANITIZER AND NOT MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        -fsanitize=undefined
        -fno-omit-frame-pointer
    )
    target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=undefined)
endif()

if(VOLTRON_ENABLE_IPO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR)
    if(IPO_SUPPORTED)
        set_property(TARGET ${PROJECT_NAME} PROPERTY
            INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
        )
    else()
        message(STATUS "IPO unavailable: ${IPO_ERROR}")
    endif()
endif()

# Specify target include directories
target_include_directories(${PROJECT_NAME} PRIVATE include src)

# Link Raylib to our executable
# Raylib's CMake configuration automatically handles platform-specific
# dependencies (like OpenGL, Cocoa/IOKit on macOS, etc.)
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE psapi)
endif()

# Copy assets folder to the build directory after building so new assets are always included
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    ${CMAKE_CURRENT_BINARY_DIR}/assets
)

include(CTest)
if(BUILD_TESTING)
    add_test(
        NAME validate_required_assets
        COMMAND ${CMAKE_COMMAND}
            -DPROJECT_ROOT=${CMAKE_CURRENT_SOURCE_DIR}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ValidateAssets.cmake
    )
endif()
