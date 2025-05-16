//
// Created by admin on 2025/5/16.
//

#ifndef TANKS_ADDSPEED_H
#define TANKS_ADDSPEED_H

#include "heads.h"
#include "Tools.h"

class AddSpeed:public Tools {
public:
    AddSpeed(sf::Vector2f pos, const sf::Texture &texture);

    void applyEffect(Tank &tank, Game &gameContext) override;

};


#endif //TANKS_ADDSPEED_H
