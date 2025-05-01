include(ExternalProject)

set(THIRDPARTY_INSTALL_FOLDER ${CMAKE_BINARY_DIR}/../sdks/installed/${CMAKE_CFG_INTDIR})

# add_subdirectory(qt6)
# set_target_properties(qt PROPERTIES FOLDER "sdks")


if (WITH_TESTS)
    add_subdirectory(gtest)
    set_target_properties(gtest PROPERTIES FOLDER "sdks")
endif()