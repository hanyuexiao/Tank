//
// Created by admin on 2025/4/30.
//
#ifndef TANKS_TOOLS_H
#define TANKS_TOOLS_H
#include "heads.h"


class Tools {
    public:
        Tools() = default;
        ~Tools() = default;
    private:
        sf::Vector2f position;
        sf::Texture Tool_img;
        sf::Sprite Tool_sprite;
        sf::FloatRect getBound() const;
        bool m_isActive;

};


#endif //TANKS_TOOLS_H
