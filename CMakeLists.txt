cmake_minimum_required(VERSION 3.21)

project(HillClimbQt VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(HillClimbQt WIN32
    src/main.cpp
    src/gamewidget.cpp
    src/gamewidget.h
    resources.qrc
)

target_link_libraries(HillClimbQt PRIVATE Qt6::Widgets)

if(MSVC)
    target_compile_options(HillClimbQt PRIVATE /W4 /permissive-)
else()
    target_compile_options(HillClimbQt PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS HillClimbQt
    RUNTIME DESTINATION .
)
