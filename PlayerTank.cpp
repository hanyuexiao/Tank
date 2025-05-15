//
// Created by admin on 2025/5/12.
//

#include "PlayerTank.h"

// PlayerTank.cpp
PlayerTank::PlayerTank(sf::Vector2f startPosition, Direction startDirection, int frameWidth, int frameHeight, int inihealth)
        : Tank(startPosition,       // startPosition
               startDirection,      // direction
               100.f,               // speed (你需要为 PlayerTank 提供一个速度值，或者从参数传入)
               frameWidth,          // frameWidth
               frameHeight,         // frameHeight
               inihealth)           // initialHealth
{
    std::cout << "PlayerTank created." << std::endl;
}