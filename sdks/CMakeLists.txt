include(ExternalProject)
include(FetchContent)

include(third_party_utils)


set(THIRDPARTY_INSTALL_FOLDER "${CMAKE_CURRENT_LIST_DIR}/_install")
message(STATUS "[SDKS][General] Install dir: ${THIRDPARTY_INSTALL_FOLDER}")

add_subdirectory(nlohmann_json)
add_subdirectory(pugixml)
set_target_properties(pugixml nlohmann_json PROPERTIES FOLDER "sdks")


if (WITH_TESTS)
    add_subdirectory(gtest)
    set_target_properties(gtest PROPERTIES FOLDER "sdks")
endif()

add_subdirectory(tracy)
