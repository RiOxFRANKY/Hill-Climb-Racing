QT += core gui widgets

CONFIG += c++17

TARGET = HillClimbGame
TEMPLATE = app

INCLUDEPATH += $$PWD/src

SOURCES += \
    src/main.cpp \
    src/physics/PhysicsWorld.cpp \
    src/terrain/SineTerrainGenerator.cpp \
    src/terrain/TerrainManager.cpp \
    src/vehicle/Car.cpp \
    src/core/InputHandler.cpp \
    src/core/GameEngine.cpp \
    src/render/SpriteManager.cpp \
    src/render/GameWidget.cpp

HEADERS += \
    src/physics/PhysicsTypes.h \
    src/physics/IPhysicsWorld.h \
    src/physics/PhysicsWorld.h \
    src/terrain/TerrainTypes.h \
    src/terrain/ITerrainGenerator.h \
    src/terrain/SineTerrainGenerator.h \
    src/terrain/TerrainManager.h \
    src/vehicle/Car.h \
    src/core/GameTypes.h \
    src/core/InputHandler.h \
    src/core/GameEngine.h \
    src/render/SpriteManager.h \
    src/render/GameWidget.h

RESOURCES += \
    resources.qrc
