# /CitySimulator/src/app/CMakeLists.txt

# Define the source files for the app
file(GLOB_RECURSE APP_SOURCES
    "main.cpp"
    "*.cpp"
    "*.h"
)

# Add executable target with the TJC_ prefix
add_library(TJC_TJamApp STATIC ${APP_SOURCES})

find_package(Qt6 6.8.3 REQUIRED COMPONENTS Core Gui Widgets)

message(STATUS "[SDKS]Found Qt6 version: ${Qt6_VERSION}")

target_precompile_headers(TJC_TJamApp PRIVATE "stdafx.h")

# Link the core static library to the app executable
target_link_libraries(TJC_TJamApp PRIVATE
    TJC_Core
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    SDL3::SDL3
)

add_subdirectory(
    "${CMAKE_SOURCE_DIR}/sdks/SDL"
    "${CMAKE_SOURCE_DIR}/.cmake/cs/sdks/installed/SDL"
    EXCLUDE_FROM_ALL
)

SET(CMAKE_AUTOMOC   ON)
SET(CMAKE_AUTOMOC_MOC_OPTIONS "-nw")

# Specify the include directories for app
target_include_directories(TJC_TJamApp PRIVATE 
    "${CMAKE_SOURCE_DIR}/CitySimulator/src/app"
    "${CMAKE_SOURCE_DIR}/CitySimulator/src/core/include"
)

# Set platform-specific flags for app if necessary
if(WIN32)
    target_compile_definitions(TJC_TJamApp PRIVATE "PLATFORM_WINDOWS")
elseif(APPLE)
    target_compile_definitions(TJC_TJamApp PRIVATE "PLATFORM_MAC")
endif()
