//
// Created by admin on 2025/5/16.
//

// AddArmor.cpp

#include "AddArmor.h"
#include "tank.h"       // 确保包含了 Tank 类的头文件
#include "game.h"       // Game 头文件可能需要，如果道具效果需要与游戏状态交互

AddArmor::AddArmor(sf::Vector2f pos, const sf::Texture &texture) : Tools(pos, texture) {
    std::cout << "Add Armor created in  (" << pos.x << ", " << pos.y << ")" << std::endl;
}

void AddArmor::applyEffect(Tank &tank, Game &gameContext) {
    // 假设 Tank 类有 getArmor 和 setArmor 方法，以及一个表示护甲上限的机制
    // 并且护甲上限为 1
    int currentArmor = tank.getArmor(); // 假设 tank.getArmor() 获取当前护甲
    int maxArmor = 1;                   // 坦克护甲上限为1

    if (currentArmor < maxArmor) {
        tank.setArmor(currentArmor + 1); // 假设 tank.setArmor() 设置新护甲
        std::cout << "Tank armor increased to: " << tank.getArmor() << std::endl;
    } else {
        std::cout << "Tank armor is already at maximum." << std::endl;
    }

    // 道具使用后通常会失效
    this->setActive(false);
    std::cout << "AddArmor effect applied. Tool deactivated." << std::endl;
}