# /CitySimulator/src/common/CMakeLists.txt

# Define the source files for the core library
file(GLOB_RECURSE COMMON_SOURCES
    "src/*.cpp"
)

# Create a static library named TJC_Core
add_library(TJS_Common STATIC ${COMMON_SOURCES})

# Specify the include directories for core
target_include_directories(TJS_Common PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
	"${PUGI_INCLUDE_DIR}"
	"${JSON_INCLUDE_DIR}"
)

target_precompile_headers(TJS_Common PRIVATE "include/common/stdafx.h")

# Set platform-specific flags for core if necessary
if(WIN32)
    target_compile_definitions(TJS_Common PRIVATE "PLATFORM_WINDOWS")
elseif(APPLE)
    target_compile_definitions(TJS_Common PRIVATE "PLATFORM_MAC")
endif()

if(WITH_TESTS)
	file(GLOB_RECURSE TEST_SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
        add_executable(common.Tests ${TEST_SOURCE_FILES})
        target_compile_definitions(common.Tests PRIVATE BENCHMARK_STATIC_DEFINE)
        target_include_directories(common.Tests PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/tests
                ${CMAKE_CURRENT_SOURCE_DIR}/include
                "${JSON_INCLUDE_DIR}"
                ${GTEST_INCLUDE_DIR}
                ${GBENCH_INCLUDE_DIR})
        target_precompile_headers(common.Tests PRIVATE "tests/stdafx.h")
        if (WIN32)
                target_link_libraries(common.Tests PRIVATE
                        TJC_Core
                        ${GTEST_LIB_DIR}/gtest.lib
                        ${GTEST_LIB_DIR}/gmock.lib
                        ${GBENCH_LIB_DIR}/benchmark.lib
                        ${GBENCH_LIB_DIR}/benchmark_main.lib
                        Shlwapi.lib   # for SHGetValueA
                        Psapi.lib     # for GetProcessMemoryInfo (used by sysinfo)
                        Winmm.lib     # sometimes needed for timing on Windows
                )
        elseif(APPLE)
                target_link_libraries(common.Tests PRIVATE
                        TJC_Core
                        ${GTEST_LIB_DIR}/libgtest.a
                        ${GTEST_LIB_DIR}/libgmock.a
                        ${GBENCH_LIB_DIR}/libbenchmark.a
                        pthread
                )
        else()
                target_link_libraries(common.Tests PRIVATE
                        TJC_Core
                        ${GTEST_LIB_DIR}/libgtest.a
                        ${GTEST_LIB_DIR}/libgmock.a
                        ${GBENCH_LIB_DIR}/libbenchmark.a
                        pthread
                )
        endif()
        add_dependencies(common.Tests gtest benchmark)
        add_test(NAME common.Tests COMMAND common.Tests)
        set_target_properties(common.Tests PROPERTIES FOLDER "Tests")
endif()
