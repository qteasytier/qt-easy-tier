# 设置最低CMake版本要求
cmake_minimum_required(VERSION 3.16)

# 设置项目名称
project(CIDRCalculator VERSION 1.0 LANGUAGES CXX)

# 启用Qt的自动MOC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 查找Qt的组件
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加头文件搜索路径（包含Qt_Items以使用QtETLineEdit）
include_directories(${CMAKE_SOURCE_DIR}/Qt_Items)

# 设置程序输出路径与主项目保持一致
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

# 构建CIDR计算器
file(GLOB_RECURSE CALCU_SRC "*.cpp" "*.h" "../Qt_QRC/*.qrc" )
# 添加Qt_Items中需要的源文件
set(QT_ITEMS_SRC
    "${CMAKE_SOURCE_DIR}/Qt_Items/qtetlineedit.cpp"
    "${CMAKE_SOURCE_DIR}/Qt_Items/qtetlineedit.h"
)
qt_add_executable(CIDRCalculator
        MANUAL_FINALIZATION
        ${CALCU_SRC}
        ${QT_ITEMS_SRC}
)
target_link_libraries(CIDRCalculator PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(CIDRCalculator PROPERTIES
        WIN32_EXECUTABLE true
)

# 安装到与主项目相同的位置
include(GNUInstallDirs)
install(TARGETS CIDRCalculator
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 自动处理 Qt 插件的依赖关系
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(CIDRCalculator)
endif()
