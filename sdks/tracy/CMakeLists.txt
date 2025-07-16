get_filename_component(CURRENT_DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

set(FETCHCONTENT_BASE_DIR ${THIRDPARTY_INSTALL_FOLDER}/${CURRENT_DIR_NAME})
include(FetchContent)

FetchContent_Declare(
    tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG v0.12.1
)

FetchContent_MakeAvailable(tracy)

if(TARGET TracyClient)
    set_target_properties(TracyClient PROPERTIES FOLDER "sdks")
endif()

set(TRACY_INCLUDE_DIR ${tracy_SOURCE_DIR}/public CACHE PATH INTERNAL)
set(TRACY_LIB_DIR ${tracy_BINARY_DIR} CACHE PATH INTERNAL)
