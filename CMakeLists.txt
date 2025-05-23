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
        Grenade.cpp
        Grenade.h
        AddArmor.cpp
        AddArmor.h
        AddAttack.cpp
        AddAttack.h
        AddAttackSpeed.cpp
        AddAttackSpeed.h
        SlowDownAI.cpp
        SlowDownAI.h
        AddSpeed.cpp
        AddSpeed.h
)

target_include_directories(Tanks PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR} # 包含项目根目录，这样可以直接 #include "game.h" 等
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party # 让编译器能找到 nlohmann/json.hpp
)

target_link_libraries(Tanks PRIVATE sfml-system sfml-window sfml-graphics sfml-network sfml-audio)

