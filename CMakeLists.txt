# 设置最低CMake版本要求
cmake_minimum_required(VERSION 3.16)

set(ProjectName QtEasyTier)

# 设置项目名称
project(${ProjectName} VERSION 1.0 LANGUAGES CXX)

# 启用Qt的自动MOC和RCC
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 设置UI、QRC文件的包含路径，用于UIC、RCC查找.ui、.qrc文件
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/Qt_UI)
set(CMAKE_AUTORCC_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/Qt_QRC)

# 查找Qt的组件
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加头文件搜索路径
include_directories(${CMAKE_SOURCE_DIR}/Qt_HEAD)
include_directories(${CMAKE_SOURCE_DIR}/Sources)

# 收集源文件
file(GLOB_RECURSE SOURCES "Sources/*.cpp" )
file(GLOB_RECURSE QT_SOURCES "Qt_SRC/*.cpp")
file(GLOB_RECURSE QT_HEAD "Qt_HEAD/*.h" "Qt_HEAD/*.hpp" "Sources/*.h" "Sources/*.hpp")
file(GLOB_RECURSE QT_UI "Qt_UI/*.ui")
file(GLOB_RECURSE QT_RESOURCES "Qt_QRC/*.qrc")
set(ALL_SOURCES ${SOURCES} ${QT_SOURCES} ${QT_HEAD} ${QT_UI} ${QT_RESOURCES})

# 设置CMke模块搜索路径
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})


# ===================================构建项目====================================

# 设置Debug构建时启用O2优化
#if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
#endif()

# 设置程序输出路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Windows系统下自动生成rc文件并嵌入manifest
if(WIN32)  # 改为判断Windows系统，而非MinGW编译器
    # 1. 定义manifest文件的绝对路径
    set(MANIFEST_FILE "${CMAKE_SOURCE_DIR}/app.manifest")

    # 2. 检查manifest文件是否存在（增加健壮性）
    if(NOT EXISTS ${MANIFEST_FILE})
        message(FATAL_ERROR "Manifest文件不存在，请检查路径：${MANIFEST_FILE}")
    endif()

    # 3. 自动生成rc文件内容
    set(GENERATED_RC_FILE "${CMAKE_BINARY_DIR}/auto_generated.rc")
    file(WRITE ${GENERATED_RC_FILE}
            "1 24 \"${MANIFEST_FILE}\""  # 直接写入rc文件的核心内容
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
else()
    qt_add_executable(${ProjectName}
            MANUAL_FINALIZATION
            ${ALL_SOURCES}
            ${QT_RESOURCES}
    )
endif()

# 链接Qt库
target_link_libraries(${ProjectName} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.${ProjectName}.)
endif()

# 设置目标属性，包括Bundle ID、版本号、短版本号、是否为Bundle、是否为Win32可执行文件
set_target_properties(${ProjectName} PROPERTIES
        ${BUNDLE_ID_OPTION}
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE true
)

# Windows下复制相关文件
if(WIN32)
    set(WINDEPLOYQT_EXECUTABLE "windeployqt.exe")

    # 构建后执行windeployqt
    #add_custom_command(TARGET ${ProjectName} POST_BUILD
    #        COMMAND ${WINDEPLOYQT_EXECUTABLE}
    #        --verbose 1
    #        --no-translations
    #        --no-compiler-runtime
    #        --no-system-d3d-compiler
    #        "$<TARGET_FILE:${ProjectName}>"
    #        COMMENT "对 ${ProjectName} 执行 windeployqt..."
    #)

    # 复制BreezeStyle文件夹中的内容到目标目录
    add_custom_command(TARGET ${ProjectName} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/BreezeStyle
            $<TARGET_FILE_DIR:${ProjectName}>
            COMMENT "复制BreezeStyle 到 ${ProjectName} 目录..."
    )

    # 复制EasyTier文件夹到目标目录
    add_custom_command(TARGET ${ProjectName} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/EasyTier
            $<TARGET_FILE_DIR:${ProjectName}>/etcore
            COMMENT "复制EasyTier 到 ${ProjectName} 目录..."
    )
endif()

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

# Windows下添加安装后部署
if(WIN32)
    install(CODE "
        message(STATUS \"开始部署Qt运行时文件...\")
        execute_process(
            COMMAND \"${WINDEPLOYQT_EXECUTABLE}\"
                --verbose 1
                --no-translations
                --no-system-d3d-compiler
                \"${CMAKE_INSTALL_PREFIX}/bin/${ProjectName}.exe\"
            WORKING_DIRECTORY \"${CMAKE_INSTALL_PREFIX}/bin\"
        )
        message(STATUS \"部署BreezeStyle...\")
        execute_process(
            COMMAND \"${CMAKE_COMMAND}\" -E copy_directory
                \"${CMAKE_SOURCE_DIR}/BreezeStyle\"
                \"${CMAKE_INSTALL_PREFIX}/bin\"
        )
        message(STATUS \"部署EasyTier...\")
        execute_process(
            COMMAND \"${CMAKE_COMMAND}\" -E copy_directory
                \"${CMAKE_SOURCE_DIR}/EasyTier\"
                \"${CMAKE_INSTALL_PREFIX}/bin/etcore\"
        )
        message(STATUS \"运行时文件部署完成\")
    ")
endif()

# 自动处理 Qt 插件的依赖关系
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${ProjectName})
endif()




