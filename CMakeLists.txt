# /CitySimulator/CMakeLists.txt

cmake_minimum_required(VERSION 3.27)
project(TJamCity)

set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")
set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CMAKE_COMMAND} -E time")

add_subdirectory(CitySimulator)