if(NOT DEFINED QTET_DAEMON_OUTPUT_DIR OR QTET_DAEMON_OUTPUT_DIR STREQUAL "")
    message(FATAL_ERROR "QTET_DAEMON_OUTPUT_DIR is required")
endif()
if(NOT DEFINED QTET_OUTPUT_DIR OR QTET_OUTPUT_DIR STREQUAL "")
    message(FATAL_ERROR "QTET_OUTPUT_DIR is required")
endif()
if(NOT DEFINED QTET_DAEMON_BIN_NAME OR QTET_DAEMON_BIN_NAME STREQUAL "")
    message(FATAL_ERROR "QTET_DAEMON_BIN_NAME is required")
endif()
if(NOT DEFINED QTET_DAEMON_RENAMED_BIN OR QTET_DAEMON_RENAMED_BIN STREQUAL "")
    message(FATAL_ERROR "QTET_DAEMON_RENAMED_BIN is required")
endif()

file(MAKE_DIRECTORY "${QTET_OUTPUT_DIR}")

set(daemon_binary "${QTET_DAEMON_OUTPUT_DIR}/${QTET_DAEMON_BIN_NAME}")
if(EXISTS "${daemon_binary}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${daemon_binary}" "${QTET_OUTPUT_DIR}/${QTET_DAEMON_RENAMED_BIN}"
        RESULT_VARIABLE copy_daemon_result
    )
    if(NOT copy_daemon_result EQUAL 0)
        message(FATAL_ERROR "Failed to copy qtet-daemon")
    endif()
    message(STATUS "Copied ${QTET_DAEMON_RENAMED_BIN}")
else()
    message(WARNING "qtet-daemon binary not found: ${daemon_binary}")
endif()

file(GLOB daemon_libraries
    "${QTET_DAEMON_OUTPUT_DIR}/*.so"
    "${QTET_DAEMON_OUTPUT_DIR}/*.dylib"
)

foreach(daemon_library IN LISTS daemon_libraries)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${daemon_library}" "${QTET_OUTPUT_DIR}"
        RESULT_VARIABLE copy_library_result
    )
    if(NOT copy_library_result EQUAL 0)
        message(FATAL_ERROR "Failed to copy daemon library: ${daemon_library}")
    endif()
    get_filename_component(daemon_library_name "${daemon_library}" NAME)
    message(STATUS "Copied ${daemon_library_name}")
endforeach()

message(STATUS "qtet-daemon runtime collection completed")
