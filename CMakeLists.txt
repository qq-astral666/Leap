cmake_minimum_required(VERSION 3.21)

project(Leap
    VERSION 1.0.0
    DESCRIPTION "Leader-key launcher for macOS: one key, then a short sequence"
    LANGUAGES CXX)

if(APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(LEAP_BUILD_TESTS "Build unit tests" ON)
option(LEAP_BUILD_APP "Build the Qt application" ON)

# --------------------------------------------------------------------------
# Core: keys, key tree, sequences, leader detection, window layouts.
# Plain C++20, no Qt, no OS -> unit-tested.
# --------------------------------------------------------------------------
add_library(leap_core STATIC
    src/core/Keys.h
    src/core/Keys.cpp
    src/core/Keymap.h
    src/core/Keymap.cpp
    src/core/Sequencer.h
    src/core/Sequencer.cpp
    src/core/Engine.h
    src/core/Engine.cpp
    src/core/Layout.h
    src/core/Layout.cpp
    src/core/HidMapping.h
    src/core/HidMapping.cpp
)
target_include_directories(leap_core PUBLIC src/core)

if(LEAP_BUILD_TESTS)
    enable_testing()
    add_executable(tst_core tests/tst_core.cpp)
    target_link_libraries(tst_core PRIVATE leap_core)
    add_test(NAME core COMMAND tst_core)
endif()

if(NOT LEAP_BUILD_APP)
    return()
endif()

# --------------------------------------------------------------------------
# Application
# --------------------------------------------------------------------------
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Quick QuickControls2)
qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(Leap MACOSX_BUNDLE
    src/main.cpp
    src/app/Config.h
    src/app/Config.cpp
    src/app/CapsLockRemap.h
    src/app/CapsLockRemap.cpp
    src/app/Controller.h
    src/app/Controller.cpp
    src/app/IconProvider.h
    src/app/IconProvider.cpp
    src/app/Tray.h
    src/app/Tray.cpp
    src/platform/Native.h
)
target_include_directories(Leap PRIVATE src/app src/platform)
target_compile_definitions(Leap PRIVATE PROJECT_VERSION_STRING="${PROJECT_VERSION}")

if(APPLE)
    target_sources(Leap PRIVATE src/platform/Native_mac.mm)
    set_source_files_properties(src/platform/Native_mac.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
    target_link_libraries(Leap PRIVATE
        "-framework AppKit"
        "-framework ApplicationServices"
        "-framework Carbon"
        "-framework CoreGraphics"
        "-framework ServiceManagement")
else()
    target_sources(Leap PRIVATE src/platform/Platform_stub.cpp)
endif()

set(LEAP_QML_FILES
    qml/HintPanel.qml
    qml/Settings.qml
    qml/Editor.qml
    qml/GeneralPage.qml
    qml/Theme.qml
    qml/Icon.qml
    qml/KeyCap.qml
    qml/KeyField.qml
    qml/ItemIcon.qml
    qml/WindowPreview.qml
    qml/PushButton.qml
    qml/TextBox.qml
    qml/TextAreaBox.qml
    qml/Select.qml
    qml/Toggle.qml
)
foreach(qml_file IN LISTS LEAP_QML_FILES)
    get_filename_component(qml_name "${qml_file}" NAME)
    # Interpreted/JIT-compiled instead of AOT C++: keeps the build independent
    # of qmlcachegen quirks between Qt versions.
    set_source_files_properties("${qml_file}" PROPERTIES
        QT_RESOURCE_ALIAS "${qml_name}"
        QT_QML_SKIP_CACHEGEN TRUE)
endforeach()
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(Leap
    URI Leap
    VERSION 1.0
    QML_FILES ${LEAP_QML_FILES}
    RESOURCES resources/AppIcon.png
)

target_link_libraries(Leap PRIVATE leap_core Qt6::Gui Qt6::Widgets Qt6::Quick Qt6::QuickControls2)

set_target_properties(Leap PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Info.plist.in"
    MACOSX_BUNDLE_GUI_IDENTIFIER "app.leap.Leap"
    MACOSX_BUNDLE_BUNDLE_NAME "Leap"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
)

# resources/AppIcon.icns is generated from AppIcon.png by scripts/package.sh.
if(APPLE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/AppIcon.icns")
    set(app_icon "${CMAKE_CURRENT_SOURCE_DIR}/resources/AppIcon.icns")
    set_source_files_properties(${app_icon} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(Leap PRIVATE ${app_icon})
    set_target_properties(Leap PROPERTIES MACOSX_BUNDLE_ICON_FILE "AppIcon.icns")
endif()
