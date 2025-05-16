//
// Created by admin on 2025/5/16.
//

#ifndef TANKS_ADDATTACK_H
#define TANKS_ADDATTACK_H

#include "Tools.h"
#include "game.h"

class AddAttack:public Tools {
    public:
         AddAttack(sf::Vector2f pos, const sf::Texture &texture);

         void applyEffect(Tank &tank,Game& gameContext) override;

};


#endif //TANKS_ADDATTACK_H
