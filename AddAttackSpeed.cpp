#include "AddAttackSpeed.h"
#include "Tank.h"   // 确保包含了 Tank 类的头文件
#include "Game.h"   // Game 头文件

AddAttackSpeed::AddAttackSpeed(sf::Vector2f pos, const sf::Texture &texture) : Tools(pos, texture) {
    // 构造函数内容
}

void AddAttackSpeed::applyEffect(Tank &tank, Game &gameContext) {
    // 攻速 * 1.5 意味着射击冷却时间变为原来的 1 / 1.5 倍
    float cooldownMultiplier = 1.0f / 1.5f;
    sf::Time duration = sf::seconds(3.0f);

    tank.activateAttackSpeedBuff(cooldownMultiplier, duration); // 假设Tank类有此方法

    this->setActive(false); // 道具使用后失效
    std::cout << "AddAttackSpeed effect applied. Tank attack speed will be *1.5 for 3 seconds. Tool deactivated." << std::endl;
}