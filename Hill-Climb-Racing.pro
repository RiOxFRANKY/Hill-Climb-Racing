QT += core gui widgets

TEMPLATE = app
TARGET = HillClimbRacing

CONFIG += c++17
CONFIG -= app_bundle

SOURCES += \
    src/main.cpp \
    src/gamewidget.cpp

HEADERS += \
    src/gamewidget.h

INCLUDEPATH += src

win32-g++ {
    QMAKE_CXXFLAGS += -Wall -Wextra -Wpedantic
}

win32-msvc {
    QMAKE_CXXFLAGS += /W4 /permissive-
}
