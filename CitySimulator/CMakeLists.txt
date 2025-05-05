# /CitySimulator/CMakeLists.txt

cmake_minimum_required(VERSION 3.27)
project(TJamSimulator)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_DIRECTORY ${CMAKE_SOURCE_DIR}/build/cs/${CMAKE_BUILD_TYPE})


# print information about core variables
message(STATUS "[INFO] CMAKE SOURCE is set to: ${CMAKE_SOURCE_DIR}")
message(STATUS "[INFO] CMAKE BINARY is set to: ${CMAKE_BINARY_DIR}")
message(STATUS "[INFO] BUILD DIRECTORY is set to: ${BUILD_DIRECTORY}")
message(STATUS "[INFO] CMake Version: ${CMAKE_VERSION}")
message(STATUS "[INFO] Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "[INFO] Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "[INFO] CMake Generator: ${CMAKE_GENERATOR}")
message(STATUS "[INFO} Build type: ${CMAKE_BUILD_TYPE}")

# Set build output directory for all platforms
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BUILD_DIRECTORY}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BUILD_DIRECTORY}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BUILD_DIRECTORY}/bin)

option(WITH_TESTS "Build tests" ON)

# add third-parties
if (WITH_TESTS)
    add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/../sdks
    ${CMAKE_BINARY_DIR}/sdks_cmake
    )

    enable_testing()
endif()

if(WIN32)
    set(CMAKE_PREFIX_PATH "C:/Qt/6.9.0/msvc2022_64")
    if(MSVC)
        add_compile_options(/Zc:__cplusplus)
    endif()
endif()

# Add subdirectories for app and core
add_subdirectory(src/app)
add_subdirectory(src/core)

# Set platform-specific flags (Windows and Mac)
if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")  # Example of Windows-specific flags
    if(MSVC)
        add_compile_options(/Zc:__cplusplus)
    endif()
elseif(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")  # Example for Mac
endif()

