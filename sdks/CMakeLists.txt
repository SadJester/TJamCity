include(ExternalProject)

set(THIRDPARTY_INSTALL_FOLDER ${CMAKE_BINARY_DIR}/../sdks/installed/${CMAKE_CFG_INTDIR})

# add_subdirectory(SDL)
# set_target_properties(SDL PROPERTIES FOLDER "sdks")


if (WITH_TESTS)
    add_subdirectory(gtest)
    set_target_properties(gtest PROPERTIES FOLDER "sdks")
endif()