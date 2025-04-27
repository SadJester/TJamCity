# /CitySimulator/CMakeLists.txt

cmake_minimum_required(VERSION 4.0)
project(TJamSimulator)
set(CMAKE_CXX_STANDARD 20)

# Set build output directory for all platforms
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../build/cs/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../build/cs/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../build/cs/bin)

option(WITH_TESTS "Build tests" ON)

# add third-parties
if (WITH_TESTS)
    add_subdirectory(
    ${CMAKE_SOURCE_DIR}/../sdks
    ${CMAKE_BINARY_DIR}/sdks_cmake
    )

    enable_testing()
endif()

# Add subdirectories for app and core
add_subdirectory(src/app)
add_subdirectory(src/core)

# Set platform-specific flags (Windows and Mac)
if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")  # Example of Windows-specific flags
elseif(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")  # Example for Mac
endif()

