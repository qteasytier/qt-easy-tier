# 设置最低CMake版本要求
cmake_minimum_required(VERSION 3.16)

set(ProjectName QtEasyTier)

# 设置项目名称
project(${ProjectName} VERSION 2.1.2 LANGUAGES CXX)

# 缓存变量：是否将配置保存到程序目录（便携模式）
set(SAVE_CONF_IN_APP_DIR false CACHE BOOL "Save configuration in application directory (portable mode)")

# 启用Qt的自动MOC和RCC
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 设置UI、QRC文件的包含路径，用于UIC、RCC查找.ui、.qrc文件
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/Qt_Items ${CMAKE_SOURCE_DIR}/Qt_Gui)
set(CMAKE_AUTORCC_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/Qt_QRC)

# 查找Qt的组件
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加头文件搜索路径
include_directories(${CMAKE_SOURCE_DIR}/Qt_HEAD)
include_directories(${CMAKE_SOURCE_DIR}/Qt_Items)
include_directories(${CMAKE_SOURCE_DIR}/Qt_Gui)
include_directories(${CMAKE_SOURCE_DIR}/SRC)
include_directories(${CMAKE_SOURCE_DIR}/ThirdParty/EasyTier/include)

# 添加EasyTier Ffi 库文件搜索路径
if(WIN32)
    link_directories(
                ${CMAKE_SOURCE_DIR}/ThirdParty/EasyTier/library
                ${CMAKE_SOURCE_DIR}/ThirdParty/EasyTier/share/windows
    )
elseif(APPLE)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        link_directories(
                    ${CMAKE_SOURCE_DIR}/ThirdParty/EasyTier/share/macos/arm64
        )
    else()
        link_directories(
                    ${CMAKE_SOURCE_DIR}/ThirdParty/EasyTier/share/macos/x86_64
        )
    endif()
else()
    link_directories(
                ${CMAKE_SOURCE_DIR}/ThirdParty/EasyTier/share/linux
    )
endif()

# 收集源文件
file(GLOB_RECURSE HEAD "SRC/*.h" "SRC/*.hpp" "Qt_Items/*.h" "Qt_Gui/*.h")
file(GLOB_RECURSE SOURCES "SRC/*.cpp" )
file(GLOB_RECURSE QT_SOURCES "Qt_Items/*.cpp" "Qt_Gui/*.cpp")
file(GLOB_RECURSE QT_UI "Qt_Items/*.ui" "Qt_Gui/*.ui")
file(GLOB_RECURSE QT_RESOURCES "Qt_QRC/*.qrc")

set(ALL_SOURCES ${SOURCES} ${QT_SOURCES} ${HEAD} ${QT_UI} ${QT_RESOURCES})

# 添加子目录（CIDR计算器）
add_subdirectory(CDIRCalculator)

# 设置程序输出路径（统一构建输出目录）
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# macOS设置程序属性
if(APPLE)
    set(MACOS_APP_ICON "${CMAKE_SOURCE_DIR}/assets/qtet.icns")
    set(MACOS_INFO_PLIST "${CMAKE_SOURCE_DIR}/assets/Info.plist")
endif()

# Windows系统下自动生成rc文件并嵌入manifest
if(WIN32)
    # 1. 定义manifest文件的绝对路径
    set(MANIFEST_FILE "${CMAKE_SOURCE_DIR}/assets/app.manifest")

    # 2. 检查manifest文件是否存在（增加健壮性）
    if(NOT EXISTS ${MANIFEST_FILE})
        message(FATAL_ERROR "Manifest文件不存在，请检查路径：${MANIFEST_FILE}")
    endif()

    # 3. 自动生成rc文件内容
    set(GENERATED_RC_FILE "${CMAKE_BINARY_DIR}/auto_generated.rc")
    file(WRITE ${GENERATED_RC_FILE}
            "1 24 \"${MANIFEST_FILE}\""  # 直接写入rc文件的核心内容
            "IDI_ICON3 ICON \"${CMAKE_SOURCE_DIR}/Qt_QRC/icon.ico\""  # 添加图标
    )

    # 4. 将自动生成的rc文件标记为CMAKE的源文件（避免IDE警告）
    set_property(SOURCE ${GENERATED_RC_FILE} PROPERTY GENERATED TRUE)
endif()

# 生成可执行程序逻辑
if(WIN32)
    qt_add_executable(${ProjectName}
            MANUAL_FINALIZATION
            ${ALL_SOURCES}
            ${GENERATED_RC_FILE}
    )
elseif(APPLE)
    qt_add_executable(${ProjectName}
            MACOSX_BUNDLE
            MANUAL_FINALIZATION
            ${ALL_SOURCES}
            ${QT_RESOURCES}
            ${MACOS_APP_ICON}
    )
else()
    qt_add_executable(${ProjectName}
            MANUAL_FINALIZATION
            ${ALL_SOURCES}
            ${QT_RESOURCES}
    )
endif()

# =============== 设置版本号与配置保存目录 ===============
target_compile_definitions(${ProjectName} PRIVATE
        PROJECT_VERSION="${PROJECT_VERSION}"
        SAVE_CONF_IN_APP_DIR=${SAVE_CONF_IN_APP_DIR}  # 是否将配置保存到程序目录
)

# ==========================================

# 链接Qt库
target_link_libraries(${ProjectName} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)

# 链接EasyTier Ffi 库
target_link_libraries(${ProjectName} PRIVATE easytier_ffi)

if(APPLE)
    # 统一使用@rpath查找FFI动态库，避免构建机绝对路径被写入二进制
    set_target_properties(${ProjectName} PROPERTIES
            BUILD_RPATH "@loader_path;@executable_path/../Frameworks"
            INSTALL_RPATH "@executable_path/../Frameworks"
    )
endif()

# 设置目标属性
if(WIN32)
    set_target_properties(${ProjectName} PROPERTIES
            ${BUNDLE_ID_OPTION}
            WIN32_EXECUTABLE true
    )
elseif(APPLE)
    get_filename_component(MACOS_APP_ICON_NAME ${MACOS_APP_ICON} NAME)
    set_target_properties(${ProjectName} PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST ${MACOS_INFO_PLIST}
            MACOSX_BUNDLE_ICON_FILE ${MACOS_APP_ICON_NAME}
            RESOURCE ${MACOS_APP_ICON}
    )
endif()

# 复制平台特定文件到构建目录
if(WIN32)
    set(WINDEPLOYQT_EXECUTABLE "windeployqt.exe")

    # 复制EasyTier文件夹到目标目录
    add_custom_command(TARGET ${ProjectName} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/ThirdParty/EasyTier/share/windows
            $<TARGET_FILE_DIR:${ProjectName}>
            COMMENT "复制EasyTier 到 ${ProjectName} 目录..."
    )
elseif(APPLE)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        set(EASYTIER_MACOS_DIR "${CMAKE_SOURCE_DIR}/ThirdParty/EasyTier/share/macos/arm64")
    else()
        set(EASYTIER_MACOS_DIR "${CMAKE_SOURCE_DIR}/ThirdParty/EasyTier/share/macos/x86_64")
    endif()

    # 复制EasyTier macOS版本到目标目录
    add_custom_command(TARGET ${ProjectName} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${EASYTIER_MACOS_DIR}
            $<TARGET_FILE_DIR:${ProjectName}>
            COMMENT "复制EasyTier macOS 到 ${ProjectName} 目录..."
    )

        # FFI动态库应放在Frameworks目录，供@rpath加载
        add_custom_command(TARGET ${ProjectName} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:${ProjectName}>/../Frameworks
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE_DIR:${ProjectName}>/libeasytier_ffi.dylib
            $<TARGET_FILE_DIR:${ProjectName}>/../Frameworks/libeasytier_ffi.dylib
            COMMENT "复制EasyTier FFI动态库到Frameworks目录..."
        )

    # 读取源dylib原始install name，用于修正主程序中的依赖引用
    set(EASYTIER_FFI_SOURCE_DYLIB "${EASYTIER_MACOS_DIR}/libeasytier_ffi.dylib")
    execute_process(
            COMMAND otool -D "${EASYTIER_FFI_SOURCE_DYLIB}"
            OUTPUT_VARIABLE EASYTIER_FFI_ID_OUTPUT
            RESULT_VARIABLE EASYTIER_FFI_ID_RESULT
            ERROR_QUIET
    )
    if(EASYTIER_FFI_ID_RESULT EQUAL 0)
        string(REPLACE "\n" ";" EASYTIER_FFI_ID_LINES "${EASYTIER_FFI_ID_OUTPUT}")
        list(LENGTH EASYTIER_FFI_ID_LINES EASYTIER_FFI_ID_LINES_LEN)
        if(EASYTIER_FFI_ID_LINES_LEN GREATER 1)
            list(GET EASYTIER_FFI_ID_LINES 1 EASYTIER_FFI_ORIGINAL_ID)
            string(STRIP "${EASYTIER_FFI_ORIGINAL_ID}" EASYTIER_FFI_ORIGINAL_ID)
        endif()
    endif()
    if(NOT EASYTIER_FFI_ORIGINAL_ID)
        set(EASYTIER_FFI_ORIGINAL_ID "/Users/tfly/code/EasyTier/target/release/deps/libeasytier_ffi.dylib")
    endif()

    # 修正FFI动态库id和主程序依赖，避免dyld按绝对路径加载
    add_custom_command(TARGET ${ProjectName} POST_BUILD
            COMMAND install_name_tool -id @rpath/libeasytier_ffi.dylib $<TARGET_FILE_DIR:${ProjectName}>/../Frameworks/libeasytier_ffi.dylib
            COMMAND install_name_tool -change ${EASYTIER_FFI_ORIGINAL_ID} @rpath/libeasytier_ffi.dylib $<TARGET_FILE:${ProjectName}>
            COMMENT "修正EasyTier FFI动态库install name和可执行文件引用..."
    )
elseif(LINUX)
    # 复制EasyTier Linux版本到目标目录
    add_custom_command(TARGET ${ProjectName} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/ThirdParty/EasyTier/share/linux
            $<TARGET_FILE_DIR:${ProjectName}>
            COMMENT "复制EasyTier Linux 到 ${ProjectName} 目录..."
    )

endif()

# 复制公共服务器列表到主程序目录
add_custom_command(TARGET ${ProjectName} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/assets/publicservers.json
        $<TARGET_FILE_DIR:${ProjectName}>/publicservers.json
        COMMENT "复制公共服务器列表到主程序目录..."
)

# 复制CIDR计算器到主程序目录
add_custom_command(TARGET ${ProjectName} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:CIDRCalculator>
        $<TARGET_FILE_DIR:${ProjectName}>
        COMMENT "复制CIDR计算器到主程序目录..."
)

# ===================================安装项目====================================

# 设置安装目录
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/Install)
# 安装目标
include(GNUInstallDirs)
install(TARGETS ${ProjectName}
        BUNDLE DESTINATION .
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 安装公共服务器列表（与可执行文件同目录）
install(FILES ${CMAKE_SOURCE_DIR}/assets/publicservers.json
        DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Windows下添加安装后部署
if(WIN32)
    # 直接调用windeployqt，最小化部署Qt运行时
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS ${QT_HOST_PATH}/bin ${QT6_HOST_PATH}/bin ${QT5_HOST_PATH}/bin)

    if(WINDEPLOYQT_EXECUTABLE)
        install(CODE "
            message(STATUS \"使用windeployqt部署Qt运行时...\")
            execute_process(
                COMMAND \"${WINDEPLOYQT_EXECUTABLE}\"
                    --no-quick
                    --no-system-d3d-compiler
                    --no-translations
                    --no-opengl-sw
                    \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/${ProjectName}.exe\"
                WORKING_DIRECTORY \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}\"
                RESULT_VARIABLE deploy_result
            )
            if(NOT deploy_result EQUAL 0)
                message(WARNING \"windeployqt执行失败，返回码: \${deploy_result}\")
            endif()
        ")
    else()
        message(WARNING "未找到windeployqt工具，跳过Qt运行时部署")
    endif()

    # 安装EasyTier目录
    install(CODE "
        message(STATUS \"部署EasyTier...\")
        execute_process(
            COMMAND \"${CMAKE_COMMAND}\" -E copy_directory
                \"${CMAKE_SOURCE_DIR}/ThirdParty/EasyTier/share/windows\"
                \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}\"
        )
    ")
elseif(APPLE)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        set(EASYTIER_INSTALL_MACOS_DIR "${CMAKE_SOURCE_DIR}/ThirdParty/EasyTier/share/macos/arm64")

        # 安装EasyTier目录
        install(CODE "
            message(STATUS \"部署EasyTier macOS arm64...\")
            execute_process(
                COMMAND \"${CMAKE_COMMAND}\" -E copy_directory
                    \"${EASYTIER_INSTALL_MACOS_DIR}\"
                    \"\${CMAKE_INSTALL_PREFIX}/${ProjectName}.app/Contents/MacOS\"
            )
        ")

        # 设置可执行权限
        install(CODE "
            message(STATUS \"设置EasyTier可执行权限...\")
            execute_process(
                COMMAND chmod +x \"\${CMAKE_INSTALL_PREFIX}/${ProjectName}.app/Contents/MacOS/easytier-core\"
            )
            execute_process(
                COMMAND chmod +x \"\${CMAKE_INSTALL_PREFIX}/${ProjectName}.app/Contents/MacOS/easytier-cli\"
            )
            execute_process(
                COMMAND chmod +x \"\${CMAKE_INSTALL_PREFIX}/${ProjectName}.app/Contents/MacOS/easytier-web-embed\"
            )
        ")

        # FFI动态库放到Frameworks目录，匹配@rpath
        install(CODE "
            message(STATUS \"部署EasyTier FFI动态库到Frameworks...\")
            execute_process(
                COMMAND \"${CMAKE_COMMAND}\" -E make_directory
                    \"\${CMAKE_INSTALL_PREFIX}/${ProjectName}.app/Contents/Frameworks\"
            )
            execute_process(
                COMMAND \"${CMAKE_COMMAND}\" -E copy_if_different
                    \"${EASYTIER_INSTALL_MACOS_DIR}/libeasytier_ffi.dylib\"
                    \"\${CMAKE_INSTALL_PREFIX}/${ProjectName}.app/Contents/Frameworks/libeasytier_ffi.dylib\"
            )
            execute_process(
                COMMAND install_name_tool -id @rpath/libeasytier_ffi.dylib
                    \"\${CMAKE_INSTALL_PREFIX}/${ProjectName}.app/Contents/Frameworks/libeasytier_ffi.dylib\"
            )
        ")
    else()
        set(EASYTIER_INSTALL_MACOS_DIR "${CMAKE_SOURCE_DIR}/ThirdParty/EasyTier/share/macos/x86_64")

        # 安装EasyTier目录
        install(CODE "
            message(STATUS \"部署EasyTier macOS x86_64...\")
            execute_process(
                COMMAND \"${CMAKE_COMMAND}\" -E copy_directory
                    \"${EASYTIER_INSTALL_MACOS_DIR}\"
                    \"\${CMAKE_INSTALL_PREFIX}/${ProjectName}.app/Contents/MacOS\"
            )
        ")

        # 设置可执行权限
        install(CODE "
            message(STATUS \"设置EasyTier可执行权限...\")
            execute_process(
                COMMAND chmod +x \"\${CMAKE_INSTALL_PREFIX}/${ProjectName}.app/Contents/MacOS/easytier-core\"
            )
            execute_process(
                COMMAND chmod +x \"\${CMAKE_INSTALL_PREFIX}/${ProjectName}.app/Contents/MacOS/easytier-cli\"
            )
            execute_process(
                COMMAND chmod +x \"\${CMAKE_INSTALL_PREFIX}/${ProjectName}.app/Contents/MacOS/easytier-web-embed\"
            )
        ")

        # FFI动态库放到Frameworks目录，匹配@rpath
        install(CODE "
            message(STATUS \"部署EasyTier FFI动态库到Frameworks...\")
            execute_process(
                COMMAND \"${CMAKE_COMMAND}\" -E make_directory
                    \"\${CMAKE_INSTALL_PREFIX}/${ProjectName}.app/Contents/Frameworks\"
            )
            execute_process(
                COMMAND \"${CMAKE_COMMAND}\" -E copy_if_different
                    \"${EASYTIER_INSTALL_MACOS_DIR}/libeasytier_ffi.dylib\"
                    \"\${CMAKE_INSTALL_PREFIX}/${ProjectName}.app/Contents/Frameworks/libeasytier_ffi.dylib\"
            )
            execute_process(
                COMMAND install_name_tool -id @rpath/libeasytier_ffi.dylib
                    \"\${CMAKE_INSTALL_PREFIX}/${ProjectName}.app/Contents/Frameworks/libeasytier_ffi.dylib\"
            )
        ")
    endif()

    # 可选：尝试使用 macdeployqt 进行部署（如果可用）
    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt)
    if(MACDEPLOYQT_EXECUTABLE)
        install(CODE "
            message(STATUS \"使用macdeployqt部署Qt运行时...\")
            execute_process(
                COMMAND \"${MACDEPLOYQT_EXECUTABLE}\"
                    \"\${CMAKE_INSTALL_PREFIX}/${ProjectName}.app\"
                WORKING_DIRECTORY \"\${CMAKE_INSTALL_PREFIX}\"
                RESULT_VARIABLE deploy_result
            )
            if(NOT deploy_result EQUAL 0)
                message(WARNING \"macdeployqt执行失败，返回码: \${deploy_result}\")
            endif()
        ")
    else()
        message(STATUS "未找到macdeployqt工具，跳过自动部署。请确保目标系统已安装Qt运行时。")
    endif()
elseif(LINUX)
    # 安装EasyTier目录
    install(CODE "
        message(STATUS \"部署EasyTier Linux...\")
        execute_process(
            COMMAND \"${CMAKE_COMMAND}\" -E copy_directory
                \"${CMAKE_SOURCE_DIR}/ThirdParty/EasyTier/share/linux\"
                \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}\"
        )
    ")

    # 设置可执行权限
    install(CODE "
        message(STATUS \"设置EasyTier可执行权限...\")
        execute_process(
            COMMAND chmod +x \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/easytier-core\"
        )
        execute_process(
            COMMAND chmod +x \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/easytier-cli\"
        )
        execute_process(
            COMMAND chmod +x \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/easytier-web-embed\"
        )
    ")

    # 可选：尝试使用 linuxdeployqt 进行部署（如果可用）
    find_program(LINUXDEPLOYQT_EXECUTABLE linuxdeployqt)
    if(LINUXDEPLOYQT_EXECUTABLE)
        install(CODE "
            message(STATUS \"使用linuxdeployqt部署Qt运行时...\")
            execute_process(
                COMMAND \"${LINUXDEPLOYQT_EXECUTABLE}\"
                    \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/${ProjectName}\"
                    -bundle-non-qt-libs
                WORKING_DIRECTORY \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}\"
                RESULT_VARIABLE deploy_result
            )
            if(NOT deploy_result EQUAL 0)
                message(WARNING \"linuxdeployqt执行失败，返回码: \${deploy_result}\")
            endif()
        ")
    else()
        message(STATUS "未找到linuxdeployqt工具，跳过自动部署。请确保目标系统已安装Qt运行时。")
    endif()
endif()

# 自动处理 Qt 插件的依赖关系
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${ProjectName})
endif()
