function(qtet_configure_daemon)
    if(NOT BUILD_WITH_DAEMON)
        message(STATUS "qtet-daemon build disabled")
        return()
    endif()

    find_program(GIT_EXECUTABLE git)
    if(NOT GIT_EXECUTABLE)
        message(WARNING "git not found, skipping qtet-daemon build. Please place qtet-daemon runtime in the output directory manually.")
        return()
    endif()

    if(NOT DEFINED CLONE_DAEMON_FROM OR CLONE_DAEMON_FROM STREQUAL "")
        set(CLONE_DAEMON_FROM "GITHUB")
    endif()
    if(CLONE_DAEMON_FROM STREQUAL "CNB")
        set(qtet_daemon_repo_url "https://cnb.cool/myqfeng/qteasytier/qtet-daemon.asio")
    else()
        set(qtet_daemon_repo_url "https://github.com/qteasytier/qtet-daemon.asio.git")
    endif()
    message(STATUS "qtet-daemon clone source: ${CLONE_DAEMON_FROM} -> ${qtet_daemon_repo_url}")

    set(QTET_DAEMON_SRC_DIR "${CMAKE_BINARY_DIR}/qtet-daemon.asio")
    set(QTET_DAEMON_BUILD_DIR "${CMAKE_BINARY_DIR}/qtet-daemon.asio/build")
    set(QTET_DAEMON_OUTPUT_DIR "${CMAKE_BINARY_DIR}/qtet-daemon.asio/build/Output")
    if(WIN32)
        set(QTET_DAEMON_BIN_NAME "qtet-daemon.exe")
    else()
        set(QTET_DAEMON_BIN_NAME "qtet-daemon")
    endif()

    if(CMAKE_CONFIGURATION_TYPES)
        set(qtet_daemon_build_type Release)
        set(qtet_daemon_build_config Release)
    else()
        set(qtet_daemon_build_type "${CMAKE_BUILD_TYPE}")
        set(qtet_daemon_build_config "${CMAKE_BUILD_TYPE}")
    endif()
    if(qtet_daemon_build_type STREQUAL "")
        set(qtet_daemon_build_type Release)
    endif()

    set(qtet_daemon_build_command
        "${CMAKE_COMMAND}"
        "-DQTET_DAEMON_REPO_URL=${qtet_daemon_repo_url}"
        "-DQTET_DAEMON_SRC_DIR=${QTET_DAEMON_SRC_DIR}"
        "-DQTET_DAEMON_BUILD_DIR=${QTET_DAEMON_BUILD_DIR}"
        "-DQTET_DAEMON_BUILD_TYPE=${qtet_daemon_build_type}"
        "-DQTET_DAEMON_BUILD_CONFIG=${qtet_daemon_build_config}"
        "-DQTET_DAEMON_GENERATOR=${CMAKE_GENERATOR}"
        "-DQTET_DAEMON_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
        "-DQTET_DAEMON_C_COMPILER=${CMAKE_C_COMPILER}"
        "-DQTET_DAEMON_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    )
    # QTET_DAEMON_VER 未定义时使用远端默认分支
    if(DEFINED QTET_DAEMON_VER AND NOT QTET_DAEMON_VER STREQUAL "")
        list(APPEND qtet_daemon_build_command "-DQTET_DAEMON_VER=${QTET_DAEMON_VER}")
    endif()
    if(DEFINED CMAKE_TOOLCHAIN_FILE AND NOT CMAKE_TOOLCHAIN_FILE STREQUAL "")
        list(APPEND qtet_daemon_build_command "-DQTET_DAEMON_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
    endif()
    list(APPEND qtet_daemon_build_command -P "${CMAKE_SOURCE_DIR}/cmake/scripts/BuildDaemon.cmake")

    message(STATUS "Building qtet-daemon runtime during configure")
    execute_process(
        COMMAND ${qtet_daemon_build_command}
        RESULT_VARIABLE qtet_daemon_build_result
    )
    if(NOT qtet_daemon_build_result EQUAL 0)
        message(FATAL_ERROR "Failed to build qtet-daemon runtime")
    endif()

    message(STATUS "Collecting qtet-daemon runtime into output directory during configure")
    execute_process(
        COMMAND "${CMAKE_COMMAND}"
                -DQTET_DAEMON_OUTPUT_DIR=${QTET_DAEMON_OUTPUT_DIR}
                -DQTET_OUTPUT_DIR=${QTET_OUTPUT_DIR}
                -DQTET_DAEMON_BIN_NAME=${QTET_DAEMON_BIN_NAME}
                -P "${CMAKE_SOURCE_DIR}/cmake/scripts/CollectDaemon.cmake"
        RESULT_VARIABLE qtet_daemon_collect_result
    )
    if(NOT qtet_daemon_collect_result EQUAL 0)
        message(FATAL_ERROR "Failed to collect qtet-daemon runtime")
    endif()
endfunction()

function(qtet_attach_daemon_to_app target_name)
    # qtet-daemon is prepared during CMake configure, not as an app build dependency.
endfunction()
