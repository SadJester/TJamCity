# /CitySimulator/src/core/CMakeLists.txt

# Define the source files for the core library
file(GLOB_RECURSE CORE_SOURCES
    "src/*.cpp"
)

# Create a static library named TJC_Core
add_library(TJC_Core STATIC ${CORE_SOURCES})

# Specify the include directories for core
target_include_directories(TJC_Core PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
	"${PUGI_INCLUDE_DIR}"
)

if (WIN32)
	target_link_libraries(TJC_Core PUBLIC ${PUGI_LIB_DIR}/pugixml.lib)
elseif(APPLE) 
	target_link_libraries(TJC_Core PUBLIC ${PUGI_LIB_DIR}/libpugixml.a)
endif()

target_precompile_headers(TJC_Core PRIVATE "include/core/stdafx.h")

# Set platform-specific flags for core if necessary
if(WIN32)
    target_compile_definitions(TJC_Core PRIVATE "PLATFORM_WINDOWS")
elseif(APPLE)
    target_compile_definitions(TJC_Core PRIVATE "PLATFORM_MAC")
endif()

if(WITH_TESTS)
	file(GLOB_RECURSE TEST_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
	add_executable(core.Tests ${TEST_SOURCE_FILES})
	target_include_directories(core.Tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests ${CMAKE_CURRENT_SOURCE_DIR}/include ${GTEST_INCLUDE_DIR})
	if (WIN32)
		target_link_libraries(core.Tests PRIVATE ${GTEST_LIB_DIR}/gtest.lib ${GTEST_LIB_DIR}/gmock.lib)
	elseif(APPLE)
		target_link_libraries(core.Tests PRIVATE ${GTEST_LIB_DIR}/libgtest.a ${GTEST_LIB_DIR}/libgmock.a)
	endif()
	add_dependencies(core.Tests gtest)
	add_test(NAME core.Tests COMMAND core.Tests)
	set_target_properties(core.Tests PROPERTIES FOLDER "Tests")
endif()
