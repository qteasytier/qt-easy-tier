if(NOT DEFINED QTET_DAEMON_REPO_URL OR QTET_DAEMON_REPO_URL STREQUAL "")
    message(FATAL_ERROR "QTET_DAEMON_REPO_URL is required")
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
message(STATUS "qtet-daemon source directory: ${QTET_DAEMON_SRC_DIR}")

if(NOT EXISTS "${QTET_DAEMON_SRC_DIR}/CMakeLists.txt")
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" clone "${QTET_DAEMON_REPO_URL}" "${QTET_DAEMON_SRC_DIR}"
        RESULT_VARIABLE clone_result
    )
    if(NOT clone_result EQUAL 0)
        message(FATAL_ERROR "Failed to clone qtet-daemon")
    endif()
else()
    message(STATUS "qtet-daemon source directory already exists, skipping clone")
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
