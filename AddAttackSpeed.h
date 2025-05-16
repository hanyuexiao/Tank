//
// Created by admin on 2025/5/16.
//

#ifndef TANKS_ADDATTACKSPEED_H
#define TANKS_ADDATTACKSPEED_H

#include "heads.h"
#include "Tools.h"

class AddAttackSpeed: public Tools {

public:
    AddAttackSpeed(sf::Vector2f pos, const sf::Texture &texture);

    void applyEffect(Tank &tank,Game& gameContext) override;
};


#endif //TANKS_ADDATTACKSPEED_H
