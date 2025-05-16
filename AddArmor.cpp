//
// Created by admin on 2025/5/16.
//

#include "AddArmor.h"

AddArmor::AddArmor(sf::Vector2f pos, const sf::Texture &texture) : Tools(pos, texture) {
        std::cout << "Add Armor created in  (" << pos.x << ", " << pos.y << ")" << std::endl;
}

void AddArmor::applyEffect(Tank &tank, Game &gameContext) {

}
