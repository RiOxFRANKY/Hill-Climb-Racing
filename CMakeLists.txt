cmake_minimum_required(VERSION 3.21)

project(HillClimbQt VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Added Concurrent here
find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)

qt_add_executable(HillClimbQt WIN32
    src/main.cpp
    src/gamewidget.cpp
    src/gamewidget.h
    src/physics/physics.cpp
    src/physics/physics.h
    src/terrain/terrain.cpp
    src/terrain/terrain.h
    src/vehicle/vehicle.cpp
    src/vehicle/vehicle.h
    src/core/game_core.cpp
    src/core/game_core.h
    src/render/game_renderer.cpp
    src/render/game_renderer.h
    resources.qrc
)

target_include_directories(HillClimbQt PRIVATE src)

# Added Qt6::Concurrent here
target_link_libraries(HillClimbQt PRIVATE Qt6::Widgets Qt6::Concurrent)

if(MSVC)
    target_compile_options(HillClimbQt PRIVATE /W4 /permissive-)
else()
    target_compile_options(HillClimbQt PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS HillClimbQt
    RUNTIME DESTINATION .
)