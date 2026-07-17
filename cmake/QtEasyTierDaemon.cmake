set(QTET_HAS_DAEMON_TARGET FALSE)

function(qtet_configure_daemon)
    set(QTET_HAS_DAEMON_TARGET FALSE PARENT_SCOPE)

    if(NOT BUILD_WITH_DAEMON)
        message(STATUS "qtet-daemon build disabled")
        return()
    endif()

    if(WIN32)
        message(STATUS "qtet-daemon Windows build is not available yet; building frontend only")
        return()
    endif()

    find_program(GIT_EXECUTABLE git)
    if(NOT GIT_EXECUTABLE)
        message(WARNING "git not found, skipping qtet-daemon build. Please place qtet-daemon runtime in the output directory manually.")
        return()
    endif()

    if(CLONE_DAEMON_FROM_GITEE)
        set(qtet_daemon_repo_url "https://gitee.com/qteasytier/qtet-daemon.git")
    else()
        set(qtet_daemon_repo_url "https://github.com/qteasytier/qtet-daemon.git")
    endif()

    set(QTET_DAEMON_SRC_DIR "${CMAKE_BINARY_DIR}/qtet-daemon" PARENT_SCOPE)
    set(QTET_DAEMON_BUILD_DIR "${CMAKE_BINARY_DIR}/qtet-daemon/build" PARENT_SCOPE)
    set(QTET_DAEMON_OUTPUT_DIR "${CMAKE_BINARY_DIR}/qtet-daemon/build/Output" PARENT_SCOPE)
    set(QTET_DAEMON_BIN_NAME "qtet-daemon" PARENT_SCOPE)
    set(QTET_DAEMON_RENAMED_BIN "qtet-daemon" PARENT_SCOPE)

    add_custom_target(qtet_daemon ALL
        COMMAND "${CMAKE_COMMAND}"
                -DQTET_DAEMON_REPO_URL=${qtet_daemon_repo_url}
                -DQTET_DAEMON_SRC_DIR=${CMAKE_BINARY_DIR}/qtet-daemon
                -DQTET_DAEMON_BUILD_DIR=${CMAKE_BINARY_DIR}/qtet-daemon/build
                -DQTET_DAEMON_BUILD_TYPE=$<IF:$<CONFIG:Debug>,Debug,Release>
                -DQTET_DAEMON_BUILD_CONFIG=$<CONFIG>
                -P "${CMAKE_SOURCE_DIR}/cmake/scripts/BuildDaemon.cmake"
        COMMENT "Building qtet-daemon runtime"
        VERBATIM
    )

    set(QTET_HAS_DAEMON_TARGET TRUE PARENT_SCOPE)
endfunction()

function(qtet_attach_daemon_to_app target_name)
    if(NOT QTET_HAS_DAEMON_TARGET)
        return()
    endif()

    add_dependencies(${target_name} qtet_daemon)

    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND "${CMAKE_COMMAND}"
                -DQTET_DAEMON_OUTPUT_DIR=${QTET_DAEMON_OUTPUT_DIR}
                -DQTET_OUTPUT_DIR=${QTET_OUTPUT_DIR}
                -DQTET_DAEMON_BIN_NAME=${QTET_DAEMON_BIN_NAME}
                -DQTET_DAEMON_RENAMED_BIN=${QTET_DAEMON_RENAMED_BIN}
                -P "${CMAKE_SOURCE_DIR}/cmake/scripts/CollectDaemon.cmake"
        COMMENT "Collecting qtet-daemon runtime into output directory"
        VERBATIM
    )
endfunction()
