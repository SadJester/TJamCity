# /CitySimulator/src/core/CMakeLists.txt

# Define the source files for the core library
file(GLOB_RECURSE CORE_SOURCES
    "src/*.cpp"
)

# Create a static library named TJC_Core
add_library(TJC_Core STATIC ${CORE_SOURCES})

# Specify the include directories for core
target_include_directories(TJC_Core PUBLIC 
    "${CMAKE_SOURCE_DIR}/src/core/include/core"
    "${CMAKE_SOURCE_DIR}/src/core/src"
)

# Set platform-specific flags for core if necessary
if(WIN32)
    target_compile_definitions(TJC_Core PRIVATE "PLATFORM_WINDOWS")
elseif(APPLE)
    target_compile_definitions(TJC_Core PRIVATE "PLATFORM_MAC")
endif()
