//
// Created by admin on 2025/4/30.
//
#ifndef TANKS_TOOLS_H
#define TANKS_TOOLS_H

#include "heads.h"

class Tank;
class Game;

class Tools {
    public:
        Tools(sf::Vector2f position, const sf::Texture& texture);
        virtual ~Tools() = default;
        sf::FloatRect getBound() const;

        virtual void applyEffect(Tank& tank,Game& gameContext) = 0;

        virtual void updata(sf::Time dt);

        void draw(sf::RenderWindow& window);

        bool isActive() const;

        void setActive(bool active);

        sf::Vector2f getPosition() const;

protected:
        sf::Vector2f m_position;

        sf::Sprite m_sprite;

        bool m_isActive;

        sf::Texture m_Texture;//假设构造时传入纹理

    sf::FloatRect getGlobalBounds();
};


#endif //TANKS_TOOLS_H
