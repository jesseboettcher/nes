cmake_minimum_required(VERSION 3.16)

project(nes_qt VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)

find_package(Qt6 6.4 REQUIRED COMPONENTS Quick)
find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()
qt_policy(SET QTP0001 OLD)

qt_add_executable(appnes_qt
    main.cpp view_update_relay.hpp menu_handler.hpp menu_handler.cpp ui_controller.hpp ui_properties.cpp ui_window.hpp ui_context.cpp ui_context.hpp sprites_model.cpp sprites_model.hpp
)

target_include_directories(appnes_qt
    PRIVATE ../
)
include_directories(../io)
include_directories(../lib)


qt_add_qml_module(appnes_qt
    URI nes_qt
    VERSION 1.0
    QML_FILES main.qml registers.qml memory.qml sprites.qml
    SOURCES ../config/flags.hpp
    SOURCES ../processor/instructions.cpp ../processor/instructions.hpp ../processor/memory.hpp ../processor/nes_ppu.cpp ../processor/nes_ppu.hpp ../processor/processor_6502.cpp ../processor/processor_6502.hpp ../processor/utils.cpp ../processor/utils.hpp ../processor/video_memory.hpp
    SOURCES ../io/linux_display.cpp ../io/joypads.cpp ../io/cartridge.cpp ../io/cartridge.hpp ../io/display.hpp ../io/files.cpp ../io/files.hpp  ../io/prompt.cpp ../io/prompt.hpp
    SOURCES ../system/nes.cpp ../system/nes.hpp
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
)

include(GNUInstallDirs)
install(TARGETS appnes_qt
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
