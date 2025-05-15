//
// Created by admin on 2025/4/30.
//

#include "heads.h"
#include "Tools.h"
#include "tank.h"

Tools::Tools(sf::Vector2f position, const sf::Texture& texture):m_position(position), m_isActive(true){
    m_sprite.setTexture(texture);
    m_sprite.setPosition(position);
    sf::FloatRect bounds = m_sprite.getGlobalBounds();
    m_sprite.setOrigin(bounds.width / 2, bounds.height / 2);
    m_sprite.setPosition(m_position);
}

void Tools::updata(sf::Time dt) {}

void Tools::draw(sf::RenderWindow& window) {
    if(m_isActive) {
        window.draw(m_sprite);
    }
}

void Tools::setActive(bool active) {
    m_isActive = active;
}

bool Tools::isActive() const {
    return false;
}

sf::Vector2f Tools::getPosition() const {
    return sf::Vector2f();
}

sf::FloatRect Tools::getBound() const {
    if(m_isActive) {
        return m_sprite.getGlobalBounds();
    } else {
        return sf::FloatRect();
    }
}


