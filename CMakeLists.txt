cmake_minimum_required(VERSION 3.16)
project(HillClimbGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(SOURCES
    src/main.cpp
    src/physics/PhysicsWorld.cpp
    src/terrain/SineTerrainGenerator.cpp
    src/terrain/TerrainManager.cpp
    src/vehicle/Car.cpp
    src/core/InputHandler.cpp
    src/core/GameEngine.cpp
    src/render/SpriteManager.cpp
    src/render/GameWidget.cpp
    resources.qrc
)

set(HEADERS
    src/physics/PhysicsTypes.h
    src/physics/IPhysicsWorld.h
    src/physics/PhysicsWorld.h
    src/terrain/TerrainTypes.h
    src/terrain/ITerrainGenerator.h
    src/terrain/SineTerrainGenerator.h
    src/terrain/TerrainManager.h
    src/vehicle/Car.h
    src/core/GameTypes.h
    src/core/InputHandler.h
    src/core/GameEngine.h
    src/render/SpriteManager.h
    src/render/GameWidget.h
)

add_executable(HillClimbGame ${SOURCES} ${HEADERS})

target_include_directories(HillClimbGame PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(HillClimbGame PRIVATE
    Qt6::Widgets
)

# Ensure assets directory is also copied directly to output directory for local disk loading
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
