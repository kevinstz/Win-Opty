cmake_minimum_required(VERSION 3.20)
project(WinOpty VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(WINOPTY_BUILD_TESTS "Build Win-Opty unit tests" ON)

set(WINOPTY_CORE_SOURCES
    src/core/restore_point.cpp
    src/core/tweak_engine.cpp
    src/core/memory_cleaner.cpp
    src/core/cache_cleaner.cpp
    # Modulos de Parte B (actualmente stubs compilables; ver TASKS.md)
    src/core/hardware_monitor.cpp
    src/core/priority_launcher.cpp
    src/core/game_mode.cpp
    src/core/overlay_hud.cpp
)

set(WINOPTY_UI_SOURCES
    # Modulos de Parte B (actualmente stubs compilables; ver TASKS.md)
    src/ui/tray_controller.cpp
    src/ui/main_window.cpp
)

add_library(winopty_core STATIC ${WINOPTY_CORE_SOURCES})
target_include_directories(winopty_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(winopty_core PUBLIC
    WINOPTY_DEFAULT_PRESETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/src/presets"
)

if(WIN32)
    target_compile_definitions(winopty_core PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(winopty_core PUBLIC advapi32 ntdll psapi shell32 ole32 user32)
endif()

if(MSVC)
    target_compile_options(winopty_core PRIVATE /W4 /utf-8)
else()
    target_compile_options(winopty_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(winopty_ui STATIC ${WINOPTY_UI_SOURCES})
target_include_directories(winopty_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(winopty_ui PUBLIC winopty_core)

add_executable(win-opty src/main.cpp)
target_link_libraries(win-opty PRIVATE winopty_core winopty_ui)

if(WINOPTY_BUILD_TESTS)
    enable_testing()
    add_executable(win-opty-tests tests/test_core_part_a.cpp)
    target_link_libraries(win-opty-tests PRIVATE winopty_core)
    add_test(NAME WinOptyCorePartATests COMMAND win-opty-tests)
endif()
