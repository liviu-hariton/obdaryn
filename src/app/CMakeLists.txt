# SPDX-License-Identifier: GPL-3.0-or-later
#
# The Obdaryn desktop application (Qt Quick).

find_package(Qt6 REQUIRED COMPONENTS Svg)

# The app icon (macOS bundle). Made from macos/icon-1024.png:
#   sips + iconutil, see docs/RELEASE.md.
set(OBDARYN_APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/macos/Obdaryn.icns")
set_source_files_properties(${OBDARYN_APP_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

qt_add_executable(obdaryn
    main.cpp
    IconProvider.h IconProvider.cpp
    ${OBDARYN_APP_ICON}
)
set_target_properties(obdaryn PROPERTIES
    OUTPUT_NAME "Obdaryn"
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "ro.obdaryn.Obdaryn"
    MACOSX_BUNDLE_BUNDLE_NAME "Obdaryn"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/macos/Info.plist.in"
    MACOSX_BUNDLE_ICON_FILE "Obdaryn.icns"
    WIN32_EXECUTABLE TRUE
)
target_compile_definitions(obdaryn PRIVATE OBDARYN_VERSION="${PROJECT_VERSION}")
target_include_directories(obdaryn PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
obdaryn_set_warnings(obdaryn)

# Singletons must be flagged before qt_add_qml_module sees them.
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(obdaryn
    URI Obdaryn
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/components/Icon.qml
        qml/components/AppButton.qml
        qml/components/NavItem.qml
        qml/components/Sidebar.qml
        qml/components/Telltale.qml
        qml/components/Sparkline.qml
        qml/components/PageHeader.qml
        qml/components/SectionTitle.qml
        qml/components/KindChip.qml
        qml/components/Chip.qml
        qml/components/KeyValue.qml
        qml/components/EmptyState.qml
        qml/components/BrandValues.qml
        qml/components/Segmented.qml
        qml/components/ToggleSwitch.qml
        qml/components/TextInput.qml
        qml/components/Toast.qml
        qml/components/ClearCodesDialog.qml
        qml/components/NotConnected.qml
        qml/components/DescriptionSource.qml
        qml/components/Dropdown.qml
        qml/pages/OverviewPage.qml
        qml/pages/ConnectView.qml
        qml/pages/CodesPage.qml
        qml/pages/ReadinessPage.qml
        qml/pages/LivePage.qml
        qml/pages/FreezeFramePage.qml
        qml/pages/VehiclePage.qml
        qml/pages/LogPage.qml
        qml/pages/SettingsPage.qml
        qml/pages/ModulesPage.qml
        qml/pages/TripPage.qml
        qml/pages/ChartsPage.qml
        qml/pages/HelpPage.qml
    SOURCES
        AppController.h AppController.cpp
        models/LiveDataModel.h models/LiveDataModel.cpp
        models/DtcModel.h models/DtcModel.cpp
        models/TrafficLogModel.h models/TrafficLogModel.cpp
        models/ModuleScanModel.h models/ModuleScanModel.cpp
        models/ChartModel.h models/ChartModel.cpp
        models/ManufacturerValuesModel.h models/ManufacturerValuesModel.cpp
        report/ReportWriter.h report/ReportWriter.cpp
)

# Fonts and icons, compiled into the binary.
file(GLOB OBDARYN_FONTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/*.ttf")
file(GLOB OBDARYN_ICONS "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/*.svg")
qt_add_resources(obdaryn "obdaryn_assets"
    PREFIX "/"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    FILES ${OBDARYN_FONTS} ${OBDARYN_ICONS})

target_link_libraries(obdaryn PRIVATE
    obdaryn_core
    Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::Svg)

# Translations: `cmake --build build --target obdaryn_lupdate` refreshes
# i18n/obdaryn_ro.ts from the sources; the .qm files are built and embedded
# under :/i18n automatically.
qt_add_translations(obdaryn
    SOURCE_TARGETS obdaryn obdaryn_core
    TS_FILE_DIR "${PROJECT_SOURCE_DIR}/i18n"
    TS_FILE_BASE obdaryn
    RESOURCE_PREFIX "/i18n"
    LUPDATE_OPTIONS -no-obsolete -locations none)
