cmake_minimum_required(VERSION 3.16)
project(QwirkleGame)

set(CMAKE_CXX_STANDARD 17)

find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

add_executable(qwirkle
    src/main.cpp
    src/Game.cpp
    src/Board.cpp
)

target_include_directories(qwirkle PRIVATE src)

target_link_libraries(qwirkle PRIVATE sfml-graphics sfml-window sfml-system)
