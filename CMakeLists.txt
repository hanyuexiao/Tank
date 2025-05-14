cmake_minimum_required(VERSION 3.28)
project(Tanks)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(SFML_DIR C:/Users/admin/CLionProjects/Tanks/SFML-2.6.2/lib/cmake/SFML)

find_package(SFML REQUIRED COMPONENTS system window graphics network audio)

add_executable(Tanks main.cpp
        tank.cpp
        tank.h
        game.cpp
        game.h
        Bullet.cpp
        Bullet.h
        Map.cpp
        Map.h
        Tools.cpp
        Tools.h
        heads.h
        common.h
        PlayerTank.cpp
        PlayerTank.h
        AITank.cpp
        AITank.h
)
target_link_libraries(Tanks PRIVATE sfml-system sfml-window sfml-graphics sfml-network sfml-audio)