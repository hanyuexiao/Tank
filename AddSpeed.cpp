// AddSpeed.cpp

#include "AddSpeed.h"
#include "Tank.h"   // 确保包含了 Tank 类的头文件
#include "Game.h"   // Game 头文件

AddSpeed::AddSpeed(sf::Vector2f pos, const sf::Texture &texture) : Tools(pos, texture) {
    // 构造函数内容
}

void AddSpeed::applyEffect(Tank &tank, Game &gameContext) {
    float speedIncreaseAmount = 100.0f;
    sf::Time duration = sf::seconds(5.0f);

    tank.activateMovementSpeedBuff(speedIncreaseAmount, duration); // 假设Tank类有此方法

    this->setActive(false); // 道具使用后失效
    std::cout << "AddSpeed effect applied. Tank movement speed increased by " << speedIncreaseAmount
              << " for " << duration.asSeconds() << " seconds. Tool deactivated." << std::endl;
}