if(NOT DEFINED QTET_DAEMON_REPO_URL OR QTET_DAEMON_REPO_URL STREQUAL "")
    message(FATAL_ERROR "QTET_DAEMON_REPO_URL is required")
endif()
# QTET_DAEMON_VER 可选：定义且非空时克隆/对齐到指定 tag，否则使用远端默认分支
set(QTET_DAEMON_HAS_TAG FALSE)
if(DEFINED QTET_DAEMON_VER AND NOT QTET_DAEMON_VER STREQUAL "")
    set(QTET_DAEMON_HAS_TAG TRUE)
endif()
if(NOT DEFINED QTET_DAEMON_SRC_DIR OR QTET_DAEMON_SRC_DIR STREQUAL "")
    message(FATAL_ERROR "QTET_DAEMON_SRC_DIR is required")
endif()
if(NOT DEFINED QTET_DAEMON_BUILD_DIR OR QTET_DAEMON_BUILD_DIR STREQUAL "")
    message(FATAL_ERROR "QTET_DAEMON_BUILD_DIR is required")
endif()

if(NOT DEFINED QTET_DAEMON_BUILD_TYPE OR QTET_DAEMON_BUILD_TYPE STREQUAL "")
    set(QTET_DAEMON_BUILD_TYPE Release)
endif()

find_program(GIT_EXECUTABLE git REQUIRED)

message(STATUS "qtet-daemon repository: ${QTET_DAEMON_REPO_URL}")
if(QTET_DAEMON_HAS_TAG)
    message(STATUS "qtet-daemon version (tag): ${QTET_DAEMON_VER}")
else()
    message(STATUS "qtet-daemon version (branch): remote default branch")
endif()
message(STATUS "qtet-daemon source directory: ${QTET_DAEMON_SRC_DIR}")

if(NOT EXISTS "${QTET_DAEMON_SRC_DIR}/CMakeLists.txt")
    set(clone_command "${GIT_EXECUTABLE}" clone --depth 1)
    if(QTET_DAEMON_HAS_TAG)
        list(APPEND clone_command --branch "${QTET_DAEMON_VER}")
    endif()
    list(APPEND clone_command "${QTET_DAEMON_REPO_URL}" "${QTET_DAEMON_SRC_DIR}")
    execute_process(
        COMMAND ${clone_command}
        RESULT_VARIABLE clone_result
    )
    if(NOT clone_result EQUAL 0)
        message(FATAL_ERROR "Failed to clone qtet-daemon")
    endif()
else()
    if(QTET_DAEMON_HAS_TAG)
        message(STATUS "qtet-daemon source directory already exists, forcing alignment to tag ${QTET_DAEMON_VER}")
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" -C "${QTET_DAEMON_SRC_DIR}" fetch --tags --force
            RESULT_VARIABLE fetch_result
        )
        if(NOT fetch_result EQUAL 0)
            message(FATAL_ERROR "Failed to fetch qtet-daemon tags")
        endif()
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" -C "${QTET_DAEMON_SRC_DIR}" checkout --detach --force "${QTET_DAEMON_VER}"
            RESULT_VARIABLE checkout_result
        )
        if(NOT checkout_result EQUAL 0)
            message(FATAL_ERROR "Failed to checkout qtet-daemon tag ${QTET_DAEMON_VER}")
        endif()
    else()
        message(STATUS "qtet-daemon source directory already exists, forcing alignment to remote default branch")
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" -C "${QTET_DAEMON_SRC_DIR}" fetch --force origin
            RESULT_VARIABLE fetch_result
        )
        if(NOT fetch_result EQUAL 0)
            message(FATAL_ERROR "Failed to fetch qtet-daemon default branch")
        endif()
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" -C "${QTET_DAEMON_SRC_DIR}" remote set-head origin -a
            RESULT_VARIABLE set_head_result
        )
        if(NOT set_head_result EQUAL 0)
            message(FATAL_ERROR "Failed to resolve qtet-daemon default branch")
        endif()
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" -C "${QTET_DAEMON_SRC_DIR}" checkout --detach --force origin/HEAD
            RESULT_VARIABLE checkout_result
        )
        if(NOT checkout_result EQUAL 0)
            message(FATAL_ERROR "Failed to checkout qtet-daemon default branch")
        endif()
    endif()
endif()

set(configure_command
    "${CMAKE_COMMAND}"
    -S "${QTET_DAEMON_SRC_DIR}"
    -B "${QTET_DAEMON_BUILD_DIR}"
    -DCMAKE_BUILD_TYPE=${QTET_DAEMON_BUILD_TYPE}
)

if(DEFINED QTET_DAEMON_GENERATOR AND NOT QTET_DAEMON_GENERATOR STREQUAL "")
    list(APPEND configure_command -G "${QTET_DAEMON_GENERATOR}")
endif()
if(DEFINED QTET_DAEMON_MAKE_PROGRAM AND NOT QTET_DAEMON_MAKE_PROGRAM STREQUAL "")
    list(APPEND configure_command -DCMAKE_MAKE_PROGRAM=${QTET_DAEMON_MAKE_PROGRAM})
endif()
if(DEFINED QTET_DAEMON_C_COMPILER AND NOT QTET_DAEMON_C_COMPILER STREQUAL "")
    list(APPEND configure_command -DCMAKE_C_COMPILER=${QTET_DAEMON_C_COMPILER})
endif()
if(DEFINED QTET_DAEMON_CXX_COMPILER AND NOT QTET_DAEMON_CXX_COMPILER STREQUAL "")
    list(APPEND configure_command -DCMAKE_CXX_COMPILER=${QTET_DAEMON_CXX_COMPILER})
endif()
if(DEFINED QTET_DAEMON_TOOLCHAIN_FILE AND NOT QTET_DAEMON_TOOLCHAIN_FILE STREQUAL "")
    list(APPEND configure_command -DCMAKE_TOOLCHAIN_FILE=${QTET_DAEMON_TOOLCHAIN_FILE})
endif()

execute_process(
    COMMAND ${configure_command}
    RESULT_VARIABLE configure_result
)
if(NOT configure_result EQUAL 0)
    message(FATAL_ERROR "Failed to configure qtet-daemon")
endif()

set(build_command "${CMAKE_COMMAND}" --build "${QTET_DAEMON_BUILD_DIR}")
if(DEFINED QTET_DAEMON_BUILD_CONFIG AND NOT QTET_DAEMON_BUILD_CONFIG STREQUAL "")
    list(APPEND build_command --config "${QTET_DAEMON_BUILD_CONFIG}")
endif()

execute_process(
    COMMAND ${build_command}
    RESULT_VARIABLE build_result
)
if(NOT build_result EQUAL 0)
    message(FATAL_ERROR "Failed to build qtet-daemon")
endif()

message(STATUS "qtet-daemon build completed")
