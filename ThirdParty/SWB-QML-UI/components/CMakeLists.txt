# Package every Swb control into the SwbControls QML module.
# The root BUILD_SHARED_LIBS option controls whether the library is shared or static.
qt_add_library(SwbControls)

# Mark SwbTheme as a singleton before registering the QML module.
set_source_files_properties(SwbTheme.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

# Single source of truth for the module's QML files; also installed for tooling.
set(swb_qml_files
        SwbBusyIndicator.qml
        SwbButton.qml
        SwbCheckBox.qml
        SwbComboBox.qml
        SwbDayOfWeekRow.qml
        SwbDelayButton.qml
        SwbDial.qml
        SwbDialog.qml
        SwbDialogButtonBox.qml
        SwbDoubleSpinBox.qml
        SwbDrawer.qml
        SwbFrame.qml
        SwbGroupBox.qml
        SwbHorizontalHeaderView.qml
        SwbIconLabel.qml
        SwbLabel.qml
        SwbMenu.qml
        SwbMenuBar.qml
        SwbMenuBarItem.qml
        SwbMenuItem.qml
        SwbMenuSeparator.qml
        SwbMonthGrid.qml
        SwbPalette.qml
        SwbPane.qml
        SwbPage.qml
        SwbPageIndicator.qml
        SwbPopup.qml
        SwbProgressBar.qml
        SwbRadioButton.qml
        SwbRangeSlider.qml
        SwbRoundButton.qml
        SwbScrollBar.qml
        SwbScrollIndicator.qml
        SwbScrollView.qml
        SwbSearchField.qml
        SwbSelectionRectangle.qml
        SwbSlider.qml
        SwbSpinBox.qml
        SwbSplitView.qml
        SwbStackView.qml
        SwbStyle.qml
        SwbSwipeView.qml
        SwbSwitch.qml
        SwbTabBar.qml
        SwbTabButton.qml
        SwbTextArea.qml
        SwbTextEditingContextMenu.qml
        SwbTextField.qml
        SwbToolBar.qml
        SwbToolButton.qml
        SwbToolSeparator.qml
        SwbToolTip.qml
        SwbTumbler.qml
        SwbVerticalHeaderView.qml
        SwbWeekNumberColumn.qml
        SwbTheme.qml
)

qt_add_qml_module(SwbControls
    URI SwbControls
    VERSION 1.0
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/SwbControls
    QML_FILES ${swb_qml_files}
)

target_link_libraries(SwbControls PRIVATE
    Qt6::Quick
    Qt6::QuickEffects
)

# ---- Installation rules for cmake --install and find_package(SwbControls) ----
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(SWB_INSTALL_QMLDIR "${CMAKE_INSTALL_DATADIR}/qml" CACHE STRING "QML 模块安装根目录")
set(SWB_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/SwbControls")
set(_swb_module_dir "${SWB_INSTALL_QMLDIR}/SwbControls")

# Backing library containing the compiled QML resources.
install(TARGETS SwbControls
    EXPORT SwbControlsTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# QML plugin loaded with the module and installed in the module directory.
install(TARGETS SwbControlsplugin
    EXPORT SwbControlsTargets
    LIBRARY DESTINATION ${_swb_module_dir}
    ARCHIVE DESTINATION ${_swb_module_dir}
    RUNTIME DESTINATION ${_swb_module_dir}
)

# Static builds generate extra resource and initialization object libraries (*_resources_N and *plugin_init).
# They are transitive dependencies and must be exported, or consumer-side find_package calls will miss targets.
get_property(_swb_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(_t IN LISTS _swb_targets)
    if(_t MATCHES "^SwbControls(_resources_[0-9]+|plugin_init)$")
        install(TARGETS ${_t}
            EXPORT SwbControlsTargets
            OBJECTS DESTINATION ${_swb_module_dir}
        )
    endif()
endforeach()

# Module metadata: the qmldir file and type information.
install(FILES
    "${CMAKE_BINARY_DIR}/SwbControls/qmldir"
    "${CMAKE_BINARY_DIR}/SwbControls/SwbControls.qmltypes"
    DESTINATION ${_swb_module_dir}
)

# QML sources next to the qmldir so qmlls/qmllint can resolve the declared types;
# the engine still loads the compiled resources (qmldir "prefer" directive).
install(FILES ${swb_qml_files} DESTINATION ${_swb_module_dir})

# Generate and install package configuration files for find_package(SwbControls).
# PROJECT_SOURCE_DIR keeps the path correct when this library is embedded with add_subdirectory.
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/SwbControlsConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/SwbControlsConfig.cmake"
    INSTALL_DESTINATION "${SWB_INSTALL_CMAKEDIR}"
    PATH_VARS SWB_INSTALL_QMLDIR
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/SwbControlsConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/SwbControlsConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/SwbControlsConfigVersion.cmake"
    DESTINATION "${SWB_INSTALL_CMAKEDIR}"
)
install(EXPORT SwbControlsTargets
    FILE SwbControlsTargets.cmake
    NAMESPACE Swb::
    DESTINATION "${SWB_INSTALL_CMAKEDIR}"
)
