if(NOT DEFINED QTET_DAEMON_OUTPUT_DIR OR QTET_DAEMON_OUTPUT_DIR STREQUAL "")
    message(FATAL_ERROR "QTET_DAEMON_OUTPUT_DIR is required")
endif()
if(NOT DEFINED QTET_OUTPUT_DIR OR QTET_OUTPUT_DIR STREQUAL "")
    message(FATAL_ERROR "QTET_OUTPUT_DIR is required")
endif()
if(NOT DEFINED QTET_DAEMON_BIN_NAME OR QTET_DAEMON_BIN_NAME STREQUAL "")
    message(FATAL_ERROR "QTET_DAEMON_BIN_NAME is required")
endif()

file(MAKE_DIRECTORY "${QTET_OUTPUT_DIR}")

set(daemon_binary "${QTET_DAEMON_OUTPUT_DIR}/${QTET_DAEMON_BIN_NAME}")
if(NOT EXISTS "${daemon_binary}")
    message(WARNING "qtet-daemon binary not found: ${daemon_binary}")
endif()

file(GLOB daemon_outputs "${QTET_DAEMON_OUTPUT_DIR}/*")

foreach(daemon_output IN LISTS daemon_outputs)
    if(IS_DIRECTORY "${daemon_output}")
        continue()
    endif()

    get_filename_component(daemon_output_name "${daemon_output}" NAME)
    get_filename_component(daemon_output_ext "${daemon_output}" EXT)
    string(TOLOWER "${daemon_output_ext}" daemon_output_ext_lower)

    if(daemon_output_name MATCHES "^tst")
        continue()
    endif()
    if(daemon_output_ext_lower STREQUAL ".a" OR daemon_output_ext_lower STREQUAL ".lib")
        continue()
    endif()

    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${daemon_output}" "${QTET_OUTPUT_DIR}"
        RESULT_VARIABLE copy_output_result
    )
    if(NOT copy_output_result EQUAL 0)
        message(FATAL_ERROR "Failed to copy daemon output: ${daemon_output}")
    endif()
    message(STATUS "Copied ${daemon_output_name}")
endforeach()

if(WIN32)
    if(NOT DEFINED QTET_WINSW_CONFIG_PATH OR QTET_WINSW_CONFIG_PATH STREQUAL "")
        message(FATAL_ERROR "QTET_WINSW_CONFIG_PATH is required on Windows")
    endif()
    if(NOT DEFINED QTET_WINSW_DOWNLOAD_URL OR QTET_WINSW_DOWNLOAD_URL STREQUAL "")
        message(FATAL_ERROR "QTET_WINSW_DOWNLOAD_URL is required on Windows")
    endif()

    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${QTET_WINSW_CONFIG_PATH}" "${QTET_OUTPUT_DIR}/DaemonInstaller.xml"
        RESULT_VARIABLE copy_winsw_config_result
    )
    if(NOT copy_winsw_config_result EQUAL 0)
        message(FATAL_ERROR "Failed to copy WinSW service config")
    endif()
    message(STATUS "Copied DaemonInstaller.xml")

    set(winsw_installer "${QTET_OUTPUT_DIR}/DaemonInstaller.exe")
    if(NOT EXISTS "${winsw_installer}")
        file(DOWNLOAD "${QTET_WINSW_DOWNLOAD_URL}" "${winsw_installer}"
            STATUS winsw_download_status
            SHOW_PROGRESS
        )
        list(GET winsw_download_status 0 winsw_download_code)
        list(GET winsw_download_status 1 winsw_download_message)
        if(NOT winsw_download_code EQUAL 0)
            file(REMOVE "${winsw_installer}")
            message(FATAL_ERROR "Failed to download WinSW: ${winsw_download_message}")
        endif()
        message(STATUS "Downloaded DaemonInstaller.exe")
    else()
        message(STATUS "DaemonInstaller.exe already exists, skipping download")
    endif()
endif()

message(STATUS "qtet-daemon runtime collection completed")
