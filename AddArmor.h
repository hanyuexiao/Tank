//
// Created by admin on 2025/5/16.
//

#ifndef TANKS_ADDARMOR_H
#define TANKS_ADDARMOR_H

#include "Tools.h"
#include "game.h"

class AddArmor:public Tools {
public:
    AddArmor(sf::Vector2f pos, const sf::Texture &texture);

    void applyEffect(Tank &tank, Game &gameContext) override;

};
#endif //TANKS_ADDARMOR_H
