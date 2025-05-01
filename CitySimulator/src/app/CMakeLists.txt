# /CitySimulator/src/app/CMakeLists.txt

# Define the source files for the app
file(GLOB APP_SOURCES
    "main.cpp"
    "*.cpp"
    "*.h"
)

# Add executable target with the TJC_ prefix
add_executable(TJC_TJamSimulator ${APP_SOURCES})

find_package(Qt6 6.8.3 REQUIRED COMPONENTS Core Gui Widgets)

message(STATUS "[SDKS]Found Qt6 version: ${Qt6_VERSION}")

# Link the core static library to the app executable
target_link_libraries(TJC_TJamSimulator PRIVATE
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

set_target_properties(TJC_TJamSimulator PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

# Specify the include directories for app
target_include_directories(TJC_TJamSimulator PRIVATE 
    "${CMAKE_SOURCE_DIR}/CitySimulator/src/app"
    "${CMAKE_SOURCE_DIR}/CitySimulator/src/core/include"
)

# Set platform-specific flags for app if necessary
if(WIN32)
    target_compile_definitions(TJC_TJamSimulator PRIVATE "PLATFORM_WINDOWS")
elseif(APPLE)
    target_compile_definitions(TJC_TJamSimulator PRIVATE "PLATFORM_MAC")
endif()
