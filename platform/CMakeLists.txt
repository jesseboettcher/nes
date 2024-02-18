cmake_minimum_required(VERSION 3.16)

project(nes_qt VERSION 0.1 LANGUAGES CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)

find_package(Qt6 6.4 REQUIRED COMPONENTS Quick)
find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets)
find_package(Qt6 6.4 REQUIRED COMPONENTS Multimedia)

qt_standard_project_setup()
qt_policy(SET QTP0001 OLD)

qt_add_executable(appnes_qt
    main.cpp view_update_relay.hpp menu_handler.hpp menu_handler.cpp ui_controller.hpp ui_properties.cpp ui_window.hpp ui_context.cpp ui_context.hpp sprites_model.cpp sprites_model.hpp
    audio_player.hpp audio_player.cpp audio_output.hpp audio_output.cpp
)

# json parsing library
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(json)

# gainput is used for cross platform USB joypad support
FetchContent_Declare(
  gainput
  GIT_REPOSITORY https://github.com/jesseboettcher/gainput.git
  GIT_TAG        master
)

set(GAINPUT_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(GAINPUT_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(GAINPUT_SAMPLES OFF CACHE BOOL "" FORCE)
set(GAINPUT_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(gainput)

# minitrace used to generate chrome trace json files
FetchContent_Declare(
  minitrace
  GIT_REPOSITORY https://github.com/hrydgard/minitrace.git
  GIT_TAG        master
)
set(MTR_ENABLED OFF CACHE BOOL "Disable minitrace" FORCE) # Disable minitrace
FetchContent_MakeAvailable(minitrace)
target_include_directories(appnes_qt PRIVATE ${minitrace_SOURCE_DIR})

target_include_directories(appnes_qt PRIVATE ../)
include_directories(../io)
include_directories(../lib)

qt_add_qml_module(appnes_qt
    URI nes_qt
    VERSION 1.0
    QML_FILES main.qml registers.qml memory.qml sprites.qml
    SOURCES ../config/flags.hpp
    SOURCES ../processor/instructions.cpp ../processor/instructions.hpp ../processor/address_bus.cpp ../processor/address_bus.hpp ../processor/nes_apu.cpp ../processor/nes_apu.hpp ../processor/nes_ppu.cpp ../processor/nes_ppu.hpp ../processor/processor_6502.cpp ../processor/processor_6502.hpp ../processor/utils.cpp ../processor/utils.hpp ../processor/ppu_address_bus.hpp
    SOURCES ../io/display.cpp ../io/joypads.cpp ../io/cartridge.cpp ../io/cartridge.hpp ../io/display.hpp ../io/files.cpp ../io/files.hpp  ../io/prompt.cpp ../io/prompt.hpp
    SOURCES ../lib/utils.cpp
    SOURCES ../system/nes.cpp ../system/nes.hpp
    SOURCES ../test/6502_tests.cpp
)

# add and configure glog
find_package(glog 0.6.0 REQUIRED)
target_link_libraries(appnes_qt PRIVATE glog::glog)
add_compile_definitions(appnes_qt GLOG_USE_GLOG_EXPORT=1)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appnes_qt PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appnes_qt
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appnes_qt
    PRIVATE Qt6::Quick Qt6::Widgets
    PRIVATE Qt6::Quick Qt6::Multimedia
    PRIVATE nlohmann_json::nlohmann_json
    PRIVATE gainput
    PRIVATE minitrace::minitrace
)

include(GNUInstallDirs)
install(TARGETS appnes_qt
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
