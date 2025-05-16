// SlowDownAI.cpp
#include "SlowDownAI.h"
#include "Game.h"
#include "AITank.h"
#include "tank.h"

SlowDownAI::SlowDownAI(sf::Vector2f pos, const sf::Texture &texture) : Tools(pos, texture) {}

void SlowDownAI::applyEffect(Tank &pickerUpperTank, Game &gameContext) {
    std::cout << "SlowDownAI effect activated (10s duration)!" << std::endl;

    std::vector<std::unique_ptr<Tank>>& allTanks = gameContext.getAllTanksForModification();
    sf::Time debuffDuration = sf::seconds(10.0f);

    float speedMultiplier = 0.3f;      // 速度变为原来的30%
    float attackCooldownFactor = 2.0f; // 冷却时间变为原来的2倍 (攻速减半)

    for (auto& tankPtr : allTanks) {
        AITank* aiTank = dynamic_cast<AITank*>(tankPtr.get());
        if (aiTank) {
            aiTank->activateSlowDebuff(speedMultiplier, attackCooldownFactor, debuffDuration);
        }
    }

    this->setActive(false);
    std::cout << "SlowDownAI effect applied to all AI tanks for 10s. Tool deactivated." << std::endl;
}