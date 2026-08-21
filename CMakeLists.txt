cmake_minimum_required(VERSION 3.15)
project(VoltronMissionGalraCypher CXX)

# Require C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# Include the src/ and include/ directories
include_directories(include src)

# Recursively find all C++ source files in the src/ directory
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

# Create the main executable target
add_executable(${PROJECT_NAME} ${SOURCES})

option(ENABLE_ADDRESS_SANITIZER "Enable AddressSanitizer for memory debugging" OFF)
if(ENABLE_ADDRESS_SANITIZER)
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /fsanitize=address /Zi)
        target_link_options(${PROJECT_NAME} PRIVATE /INCREMENTAL:NO)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
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
# Added Core singletons (GameManager, AudioManager)
