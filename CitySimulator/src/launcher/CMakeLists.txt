# /CitySimulator/src/app/CMakeLists.txt

# Define the source files for the app
file(GLOB_RECURSE APP_SOURCES
    "main.cpp"
    "*.cpp"
    "*.h"
)

# Add executable target with the TJC_ prefix
add_executable(TJC_TJamSimulator ${APP_SOURCES})

# Link the core static library to the app executable
target_link_libraries(TJC_TJamSimulator PRIVATE
    TJC_TJamApp
)

SET(CMAKE_AUTOMOC   ON)
SET(CMAKE_AUTOMOC_MOC_OPTIONS "-nw")

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
