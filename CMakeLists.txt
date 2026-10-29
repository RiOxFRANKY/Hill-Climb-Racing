cmake_minimum_required(VERSION 3.21)

project(HillClimbQt VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(HillClimbQt WIN32
    src/main.cpp
    src/physics/physics.cpp
    src/physics/physics.h
    src/terrain/terrain.cpp
    src/terrain/terrain.h
    src/vehicle/vehicle.cpp
    src/vehicle/vehicle.h
    src/core/game_core.cpp
    src/core/game_core.h
    src/render/terrain_raster.cpp
    src/render/terrain_raster.h
    src/quick/sprite_provider.cpp
    src/quick/sprite_provider.h
    resources.qrc
)

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(HillClimbQt
    URI HillClimb
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/ShadowText.qml
        qml/HudLabel.qml
        qml/Chain.qml
        qml/Background.qml
        qml/FinishLine.qml
        qml/Pickups.qml
        qml/Car.qml
        qml/Hud.qml
        qml/Speedometer.qml
        qml/Pedals.qml
        qml/ZoneBanner.qml
        qml/GameOverlay.qml
        qml/StartMenu.qml
        qml/ControlsPanel.qml
    SOURCES
        src/quick/game_controller.cpp
        src/quick/game_controller.h
        src/quick/terrain_item.cpp
        src/quick/terrain_item.h
)

target_include_directories(HillClimbQt PRIVATE src src/quick)

target_link_libraries(HillClimbQt PRIVATE Qt6::Quick)

set_target_properties(HillClimbQt PROPERTIES
    QT_ANDROID_PACKAGE_NAME org.hcr.hillclimb
    QT_ANDROID_APP_NAME "Hill Climb"
)

if(MSVC)
    target_compile_options(HillClimbQt PRIVATE /W4 /permissive-)
else()
    target_compile_options(HillClimbQt PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS HillClimbQt
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
