cmake_minimum_required(VERSION 3.16)

# Phiên bản: Tham số truyền vào (-DSENKEY_VERSION=...) > Git tag > Giá trị mặc định
if(NOT DEFINED SENKEY_VERSION OR SENKEY_VERSION STREQUAL "")
    find_package(Git QUIET)
    if(GIT_FOUND)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --match "v*" --always
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_TAG_RAW
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        if(GIT_TAG_RAW MATCHES "^v?([0-9]+\\.[0-9]+.*)")
            string(REGEX REPLACE "^v" "" SENKEY_VERSION "${GIT_TAG_RAW}")
        endif()
    endif()
endif()

if(NOT DEFINED SENKEY_VERSION OR SENKEY_VERSION STREQUAL "")
    set(SENKEY_VERSION "0.2.2")
endif()

# Trích xuất định dạng số thuần cho CMake project(...)
if(SENKEY_VERSION MATCHES "^([0-9]+(\\.[0-9]+)+)")
    set(SENKEY_NUMERIC_VERSION "${CMAKE_MATCH_1}")
else()
    set(SENKEY_NUMERIC_VERSION "0.1.4")
endif()

project(SenKey VERSION ${SENKEY_NUMERIC_VERSION} LANGUAGES CXX C)

add_compile_definitions(SENKEY_VERSION_STR="${SENKEY_VERSION}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_ASAN "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(BUILD_TESTS "Build test suite" ON)

if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address,undefined)
endif()

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wno-unused-parameter -O2)
endif()

# UniKey Core engine static library
set(UNIKEY_CORE_SOURCES
    UniKeyCore/byteio.cpp
    UniKeyCore/charset.cpp
    UniKeyCore/convert.cpp
    UniKeyCore/data.cpp
    UniKeyCore/inputproc.cpp
    UniKeyCore/mactab.cpp
    UniKeyCore/pattern.cpp
    UniKeyCore/ukengine.cpp
)

add_library(UniKeyCore STATIC ${UNIKEY_CORE_SOURCES})
target_include_directories(UniKeyCore PUBLIC UniKeyCore)

# SenKey daemon shared core library
set(SENKEY_CORE_SOURCES
    Source/Config.cpp
    Source/EngineWrapper.cpp
    Source/Hotplug.cpp
    Source/Ipc.cpp
    Source/Logger.cpp
    Source/Macro.cpp
)

add_library(SenKeyCoreLib STATIC ${SENKEY_CORE_SOURCES})
target_include_directories(SenKeyCoreLib PUBLIC Source UniKeyCore)
target_link_libraries(SenKeyCoreLib PUBLIC UniKeyCore pthread)

# Main Daemon Binary
add_executable(senkey Source/Main.cpp)
target_link_libraries(senkey PRIVATE SenKeyCoreLib UniKeyCore pthread)

# GUI Binary (requires GTK3)
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(GTK3 QUIET gtk+-3.0)
endif()

if(GTK3_FOUND)
    # Compile GResource (icons embedded into binary)
    find_program(GLIB_COMPILE_RESOURCES glib-compile-resources REQUIRED)

    set(GRESOURCE_XML "${CMAKE_SOURCE_DIR}/icons/senkey.gresource.xml")
    set(GRESOURCE_C  "${CMAKE_BINARY_DIR}/SenkeyResources.c")

    # Collect all icon files so CMake knows when to re-run
    file(GLOB ICON_FILES "${CMAKE_SOURCE_DIR}/icons/*.png")

    add_custom_command(
        OUTPUT  ${GRESOURCE_C}
        COMMAND ${GLIB_COMPILE_RESOURCES}
                --target=${GRESOURCE_C}
                --sourcedir=${CMAKE_SOURCE_DIR}/icons
                --generate-source
                ${GRESOURCE_XML}
        DEPENDS ${GRESOURCE_XML} ${ICON_FILES}
        COMMENT "Compiling GResource icons into SenkeyResources.c"
    )

    set(SENKEY_GUI_SOURCES
        ${GRESOURCE_C}
        Source/GUI/SilkIcons.cpp
        Source/GUI/AboutDialog.cpp
        Source/GUI/BasicSection.cpp
        Source/GUI/AdvancedSection.cpp
        Source/GUI/ButtonBar.cpp
        Source/GUI/MainWindow.cpp
        Source/Tray/TrayIcons.cpp
        Source/Tray/TrayBackend.cpp
        Source/Tray/TrayMenu.cpp
        Source/Tray/TrayManager.cpp
        Source/GUI/MainGui.cpp
    )

    add_executable(senkey-gui ${SENKEY_GUI_SOURCES})
    target_include_directories(senkey-gui PRIVATE
        Source
        Source/GUI
        Source/Tray
        UniKeyCore
        ${GTK3_INCLUDE_DIRS}
    )
    target_compile_options(senkey-gui PRIVATE ${GTK3_CFLAGS_OTHER})
    target_link_libraries(senkey-gui PRIVATE
        SenKeyCoreLib
        ${GTK3_LIBRARIES}
        ${CMAKE_DL_LIBS}
        pthread
    )

    message(STATUS "GTK3 found. Target 'senkey-gui' enabled (AppIndicator dynamic loading via dlopen).")
else()
    message(WARNING "GTK3 not found. Target 'senkey-gui' will not be built.")
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()

# Installation rules
install(TARGETS senkey DESTINATION bin)
if(TARGET senkey-gui)
    install(TARGETS senkey-gui DESTINATION bin)
endif()
