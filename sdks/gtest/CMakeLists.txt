set(GTEST_INSTALL_DIR ${THIRDPARTY_INSTALL_FOLDER}/gtest)
set(GTEST_INCLUDE_DIR ${GTEST_INSTALL_DIR}/include CACHE PATH INTERNAL)
set(GTEST_LIB_DIR ${GTEST_INSTALL_DIR}/lib CACHE PATH INTERNAL)

message(STATUS "[SDKS][GTest] Install dir: ${GTEST_INSTALL_DIR}")
message(STATUS "[SDKS][GTest] Library dir: ${GTEST_LIB_DIR}")
message(STATUS "[SDKS][GTest] Include dir: ${GTEST_INCLUDE_DIR}")

ExternalProject_Add(gtest 
	GIT_REPOSITORY https://github.com/google/googletest GIT_TAG "v1.16.0"
	UPDATE_COMMAND ""
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${GTEST_INSTALL_DIR} -Dgtest_force_shared_crt=ON)