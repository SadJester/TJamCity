# /CitySimulator/CMakeLists.txt

cmake_minimum_required(VERSION 3.27)
project(TJamCity)

# перенаправляем служебные данные в {root}/.cmake
set(CMAKE_CACHEFILE_DIR "${CMAKE_SOURCE_DIR}/.cmake/cs" CACHE PATH "" FORCE)
set(CMAKE_FILES_DIRECTORY "${CMAKE_CACHEFILE_DIR}/CMakeFiles" CACHE PATH "" FORCE)

# Put artefacts under build/bin/<Config> and build/lib/<Config>
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/lib")

# For multi-config generators (Visual Studio, Ninja Multi-Config),
# ensure per-config subfolders, without duplicating the config name.
if(CMAKE_CONFIGURATION_TYPES)
  foreach(cfg ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER "${cfg}" cfgU)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${cfgU} "${CMAKE_SOURCE_DIR}/build/${cfg}/bin")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${cfgU} "${CMAKE_SOURCE_DIR}/build/${cfg}/bin")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${cfgU} "${CMAKE_SOURCE_DIR}/build/${cfg}/lib")
  endforeach()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")
set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CMAKE_COMMAND} -E time")

add_subdirectory(CitySimulator)