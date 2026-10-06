QT += core gui widgets

TEMPLATE = app
TARGET = HillClimbRacing

CONFIG += c++17
CONFIG -= app_bundle

SOURCES += \
    src/main.cpp \
    src/gamewidget.cpp \
    src/physics/physics.cpp \
    src/terrain/terrain.cpp \
    src/vehicle/vehicle.cpp \
    src/core/game_core.cpp \
    src/render/game_renderer.cpp

HEADERS += \
    src/gamewidget.h \
    src/physics/physics.h \
    src/terrain/terrain.h \
    src/vehicle/vehicle.h \
    src/core/game_core.h \
    src/render/game_renderer.h

RESOURCES += resources.qrc

INCLUDEPATH += src

win32-g++ {
    QMAKE_CXXFLAGS += -Wall -Wextra -Wpedantic
}

win32-msvc {
    QMAKE_CXXFLAGS += /W4 /permissive-
}
