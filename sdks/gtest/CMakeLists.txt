add_external_project(
    MODULE_NAME gtest
    VAR_NAME GTEST
    GIT_REPOSITORY https://github.com/google/googletest
    GIT_TAG "v1.16.0"
    CMAKE_ARGS -Dgtest_force_shared_crt=ON
)
