# Spider Download Manager — Windows x64, C++17 + Qt 6 Widgets + WinHTTP.
#
# Based on FLEX Download Manager by Osama96gi (Apache-2.0), rebranded as
# Spider. Upstream README describes a build.bat; this CMake file is the
# build system for this fork (the upstream repo ships no build file).
#
# Build (GitHub Actions windows-latest, or local VS 2022+):
#   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
#   cmake --build build --config Release
#   windeployqt --release "build/Spider Download Manager.exe"
#
# Requirements: Qt 6.5+ with Widgets (tested upstream with Qt 6.8.3,
# win64_msvc2022_64), CMake >= 3.21, Ninja, MSVC (Desktop C++ workload).
# The download engine itself uses only inbox Windows libraries.
cmake_minimum_required(VERSION 3.21)
project(SpiderDownloadManager VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Unicode build (the sources guard these themselves in common.h; defining
# them here matches what Qt projects do on Windows).
add_compile_definitions(UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)

if(MSVC)
    # Warning-clean build, per the upstream README.
    add_compile_options(/W4 /permissive- /utf-8 /EHsc)
endif()

# Qt modules proven by the #includes across src/:
#   Core, Gui, Widgets — the whole UI (QMainWindow, QDialog, QTableWidget...)
#   Network — QTcpServer/QTcpSocket (Chrome bridge) + QNetworkAccessManager.
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

# The GUI application. qt_add_executable links Qt6::EntryPoint, which
# provides the WinMain entry point for the int main() in src/main.cpp.
# WIN32 marks it a GUI (WINDOWS-subsystem) exe so Windows does NOT open
# a console/cmd window next to the app.
qt_add_executable(SpiderDownloadManager WIN32
    src/main.cpp
    src/bridge.cpp
    src/common.cpp
    src/config.cpp
    src/download.cpp
    src/http.cpp
    src/localization.cpp
    src/ui_dialogs.cpp
    src/ui_main.cpp
    src/updater.cpp
    src/url.cpp
    src/ytdlp.cpp
    src/resources.qrc
    src/app.rc
)

target_link_libraries(SpiderDownloadManager PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    winhttp
    shlwapi
    shell32
    ole32
    user32
    gdi32
    advapi32
)

set_target_properties(SpiderDownloadManager PROPERTIES
    OUTPUT_NAME "Spider Download Manager"
)
