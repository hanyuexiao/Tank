//
// Created by admin on 2025/5/14.
//

#ifndef TANKS_GRENADE_H
#define TANKS_GRENADE_H

#include "Tools.h"
#include "game.h"

class GrenadeTool:public Tools
{
public:
    GrenadeTool(sf::Vector2f pos,const sf::Texture &texture);

    void applyEffect(Tank& tank,Game& gameContext) override;

};


#endif //TANKS_GRENADE_H
