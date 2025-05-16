//
// Created by admin on 2025/5/16.
//

#ifndef TANKS_SLOWDOWNAI_H
#define TANKS_SLOWDOWNAI_H

#include "heads.h"
#include "Tools.h"

class SlowDownAI:public Tools {
public:
    SlowDownAI(sf::Vector2f pos, const sf::Texture &texture);

    void applyEffect(Tank &tank,Game& gameContext) override;

};


#endif //TANKS_SLOWDOWNAI_H
