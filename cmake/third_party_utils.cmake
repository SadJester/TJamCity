function(define_thirdparty_paths PREFIX FOLDER_NAME)
    if(NOT FOLDER_NAME OR NOT PREFIX)
        message(FATAL_ERROR "define_thirdparty_paths requires PREFIX and FOLDER_NAME")
    endif()

    set(_SRC_DIR     ${THIRDPARTY_INSTALL_FOLDER}/${FOLDER_NAME}/_src)
    set(_BUILD_DIR   ${THIRDPARTY_INSTALL_FOLDER}/${FOLDER_NAME}/_build)
    set(_INSTALL_DIR ${THIRDPARTY_INSTALL_FOLDER}/${FOLDER_NAME}/_install)
    set(_INCLUDE_DIR ${_INSTALL_DIR}/include)
    set(_LIB_DIR     ${_INSTALL_DIR}/lib)

    # Export to parent scope
    set(${PREFIX}_SRC_DIR     ${_SRC_DIR}     PARENT_SCOPE)
    set(${PREFIX}_BUILD_DIR   ${_BUILD_DIR}   PARENT_SCOPE)
    set(${PREFIX}_INSTALL_DIR ${_INSTALL_DIR} PARENT_SCOPE)
    set(${PREFIX}_INCLUDE_DIR ${_INCLUDE_DIR} CACHE PATH INTERNAL)
    set(${PREFIX}_LIB_DIR     ${_LIB_DIR}     CACHE PATH INTERNAL)
endfunction()


function(add_external_project)
    set(options)
    set(oneValueArgs MODULE_NAME GIT_REPOSITORY GIT_TAG VAR_NAME)
    set(multiValueArgs CMAKE_ARGS)

    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_MODULE_NAME OR NOT ARG_GIT_REPOSITORY OR NOT ARG_GIT_TAG)
        message(FATAL_ERROR "add_external_project requires MODULE_NAME, GIT_REPOSITORY, GIT_TAG and VAR_NAME")
    endif()

    define_thirdparty_paths(${ARG_VAR_NAME} ${ARG_MODULE_NAME})

    # Log resolved paths
    message(STATUS "[SDKS][${ARG_MODULE_NAME}] Vars starts with \"${ARG_VAR_NAME}\"")
    message(STATUS "[SDKS][${ARG_MODULE_NAME}] paths:")
    message(STATUS "[SDKS][${ARG_MODULE_NAME}] Source  = ${${ARG_VAR_NAME}_SRC_DIR}")
    message(STATUS "[SDKS][${ARG_MODULE_NAME}] Build   = ${${ARG_VAR_NAME}_BUILD_DIR}")
    message(STATUS "[SDKS][${ARG_MODULE_NAME}] Install = ${${ARG_VAR_NAME}_INSTALL_DIR}")
    message(STATUS "[SDKS][${ARG_MODULE_NAME}] Include = ${${ARG_VAR_NAME}_INCLUDE_DIR}")
    message(STATUS "[SDKS][${ARG_MODULE_NAME}] Lib     = ${${ARG_VAR_NAME}_LIB_DIR}")

    ExternalProject_Add(${ARG_MODULE_NAME}
        PREFIX ${${ARG_VAR_NAME}_BUILD_DIR}
        GIT_REPOSITORY ${ARG_GIT_REPOSITORY}
        GIT_TAG ${ARG_GIT_TAG}
        SOURCE_DIR ${${ARG_VAR_NAME}_SRC_DIR}
        BINARY_DIR ${${ARG_VAR_NAME}_BUILD_DIR}
        INSTALL_DIR ${${ARG_VAR_NAME}_INSTALL_DIR}
        UPDATE_COMMAND ""
        CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX:PATH=${${ARG_VAR_NAME}_INSTALL_DIR}
            ${ARG_CMAKE_ARGS}
    )
endfunction()
