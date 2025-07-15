function(define_thirdparty_paths PREFIX)
    set(_SRC_DIR     ${THIRDPARTY_INSTALL_FOLDER}/${PREFIX}/_src)
    set(_BUILD_DIR   ${THIRDPARTY_INSTALL_FOLDER}/${PREFIX}/_build)
    set(_INSTALL_DIR ${THIRDPARTY_INSTALL_FOLDER}/${PREFIX}/_install)
    set(_INCLUDE_DIR ${_INSTALL_DIR}/include)
    set(_LIB_DIR     ${_INSTALL_DIR}/lib)

    # Log resolved paths
    message(STATUS "[SDKS][${CURRENT_DIR_NAME}] paths:")
    message(STATUS "[SDKS][${CURRENT_DIR_NAME}] Source  = ${_SRC_DIR}")
    message(STATUS "[SDKS][${CURRENT_DIR_NAME}] Build   = ${_BUILD_DIR}")
    message(STATUS "[SDKS][${CURRENT_DIR_NAME}] Install = ${_INSTALL_DIR}")
    message(STATUS "[SDKS][${CURRENT_DIR_NAME}] Include = ${_INCLUDE_DIR}")
    message(STATUS "[SDKS][${CURRENT_DIR_NAME}] Lib     = ${_LIB_DIR}")

    # Export to parent scope
    set(${PREFIX}_SRC_DIR     ${_SRC_DIR}     PARENT_SCOPE)
    set(${PREFIX}_BUILD_DIR   ${_BUILD_DIR}   PARENT_SCOPE)
    set(${PREFIX}_INSTALL_DIR ${_INSTALL_DIR} PARENT_SCOPE)
    set(${PREFIX}_INCLUDE_DIR ${_INCLUDE_DIR} CACHE PATH INTERNAL)
    set(${PREFIX}_LIB_DIR     ${_LIB_DIR}     CACHE PATH INTERNAL)
endfunction()


function(add_external_project)
    set(options)
    set(oneValueArgs MODULE_NAME GIT_REPOSITORY GIT_TAG)
    set(multiValueArgs CMAKE_ARGS)

    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_MODULE_NAME OR NOT ARG_GIT_REPOSITORY OR NOT ARG_GIT_TAG)
        message(FATAL_ERROR "add_external_project requires MODULE_NAME, GIT_REPOSITORY, and GIT_TAG")
    endif()

    define_thirdparty_paths(${ARG_MODULE_NAME})

    ExternalProject_Add(${ARG_MODULE_NAME}
        PREFIX ${${ARG_MODULE_NAME}_BUILD_DIR}
        GIT_REPOSITORY ${ARG_GIT_REPOSITORY}
        GIT_TAG ${ARG_GIT_TAG}
        SOURCE_DIR ${${ARG_MODULE_NAME}_SRC_DIR}
        BINARY_DIR ${${ARG_MODULE_NAME}_BUILD_DIR}
        INSTALL_DIR ${${ARG_MODULE_NAME}_INSTALL_DIR}
        UPDATE_COMMAND ""
        CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX:PATH=${${ARG_MODULE_NAME}_INSTALL_DIR}
            ${ARG_CMAKE_ARGS}
    )
endfunction()
