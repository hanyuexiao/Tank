//
// Created by admin on 2025/4/30.
//

#include "heads.h"
#include "Tools.h"
#include "tank.h"

Tools::Tools(sf::Vector2f position, const sf::Texture& texture):m_position(position),m_Texture(texture),m_isActive(true){
    m_sprite.setTexture(m_Texture);
    m_sprite.setPosition(position);
    sf::FloatRect bounds = m_sprite.getGlobalBounds();
    m_sprite.setOrigin(bounds.width / 2, bounds.height / 2);
    m_sprite.setPosition(m_position);
    m_lifetime = sf::seconds(20.f); // 默认生命周期为10秒
    m_age = sf::Time::Zero;
}

Tools::Tools(sf::Vector2f position, const sf::Texture& texture, sf::Time lifetime)
        : m_position(position),
          m_Texture(texture),
          m_isActive(true),
          m_lifetime(lifetime), // 使用传入的生命周期
          m_age(sf::Time::Zero) {
    m_sprite.setTexture(m_Texture);
    sf::FloatRect bounds = m_sprite.getLocalBounds();
    m_sprite.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    m_sprite.setPosition(m_position);
}

void Tools::update(sf::Time dt) {
    if (!m_isActive) {
        return;
    }

    m_age += dt; // 累加道具已存在的时间

    if (m_age >= m_lifetime) { // 如果道具已超过其生命周期
        setActive(false);      // 将道具设为不活动状态
        std::cout << "Tool at (" << getPosition().x << ", " << getPosition().y << ") timed out and disappeared." << std::endl;
    }
    // 其他特定于道具的更新逻辑可以放在这里或派生类中
}

void Tools::draw(sf::RenderWindow& window) {
    if(m_isActive) {
        window.draw(m_sprite);
    }
}

void Tools::setActive(bool active) {
    m_isActive = active;
}

bool Tools::isActive() const {
    return m_isActive;
}

sf::Vector2f Tools::getPosition() const {
    return m_position;
}

sf::FloatRect Tools::getBound() const {
    if(m_isActive) {
        return m_sprite.getGlobalBounds();
    } else {
        return sf::FloatRect();
    }
}


