//
// Created by admin on 2025/5/16.
//

#include "AddAttack.h"

AddAttack::AddAttack(sf::Vector2f pos, const sf::Texture &texture) : Tools(pos, texture) {

}

void AddAttack::applyEffect(Tank &tank, Game &gameContext) {
    float multiplier = 2.0f;
    sf::Time duration = sf::seconds(5.0f);

    tank.activateAttackBuff(multiplier, duration);

    this->setActive(false);

    std::cout << "AddAttack effect applied" << std::endl;
}
