add_external_project(
    MODULE_NAME benchmark
    VAR_NAME GBENCH
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG "v1.8.3"
    CMAKE_ARGS -DBENCHMARK_ENABLE_TESTING=OFF
)
