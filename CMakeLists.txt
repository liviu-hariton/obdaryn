# SPDX-License-Identifier: GPL-3.0-or-later
#
# Obdaryn — read-only OBD-II diagnostics for ELM327-compatible adapters.
#
# Layout:
#   src/core   obdaryn_core static library: transport, ELM327 session, safety
#              guard, OBD-II decoders. Depends only on QtCore + QtNetwork so it
#              can be reused unchanged on desktop and mobile.
#   src/app    the Qt Quick (QML) desktop application.
#   tests      Qt Test unit tests (no adapter or car needed).
#   i18n       translation sources (.ts) — English is the source language,
#              Romanian is the first translation.

cmake_minimum_required(VERSION 3.21)

project(Obdaryn
    VERSION 0.4.8
    DESCRIPTION "Read-only OBD-II diagnostics for ELM327-compatible adapters"
    HOMEPAGE_URL "https://github.com/liviu-hariton/obdaryn"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Network Gui Qml Quick QuickControls2 LinguistTools)

# Enables AUTOMOC/AUTORCC, sets sane defaults and declares the UI languages.
qt_standard_project_setup(
    REQUIRES 6.8
    I18N_SOURCE_LANGUAGE en
    I18N_TRANSLATED_LANGUAGES ro)

# Treat warnings seriously, but only for our own targets.
function(obdaryn_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_subdirectory(src/core)
add_subdirectory(src/cli)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/app/CMakeLists.txt")
    add_subdirectory(src/app)
endif()

option(OBDARYN_BUILD_TESTS "Build the unit tests" ON)
if(OBDARYN_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()
