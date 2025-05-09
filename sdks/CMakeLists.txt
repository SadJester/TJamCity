include(ExternalProject)
include(FetchContent)


if(WIN32)
    set(THIRDPARTY_INSTALL_FOLDER ${CMAKE_BINARY_DIR}/../sdks/installed)
else()
    set(THIRDPARTY_INSTALL_FOLDER ${CMAKE_BINARY_DIR}/../sdks/installed/${CMAKE_CFG_INTDIR})
endif()


add_subdirectory(nlohmann_json)
add_subdirectory(pugixml)
set_target_properties(pugixml nlohmann_json PROPERTIES FOLDER "sdks")


if (WITH_TESTS)
    add_subdirectory(gtest)
    set_target_properties(gtest PROPERTIES FOLDER "sdks")
endif()