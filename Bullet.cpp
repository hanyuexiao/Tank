// Bullet.cpp
#include "Bullet.h"
#include <iostream>

Bullet::Bullet(const sf::Texture& texture,
               sf::Vector2f startPosition,
               Direction tankDirectionEnum,
               sf::Vector2f flyDirectionVec,
               int damage,
               float speed,
               int type)
        : m_position(startPosition),
          m_flyDirection(normalize(flyDirectionVec)), // 确保飞行向量被标准化
          m_tankDirection(tankDirectionEnum),
          m_speed(speed),
          m_damage(damage),
          m_type(type),
          m_isAlive(true)
{
    m_sprite.setTexture(texture); // 直接使用传入的、已经是正确方向的纹理
    m_sprite.setPosition(m_position);

    // 因为纹理已经是特定方向的了，所以通常不需要再对 m_sprite 进行 setRotation()
    // 如果需要，可以设置原点到中心，使定位更精确
    // sf::FloatRect bounds = m_sprite.getLocalBounds();
    // m_sprite.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    // m_sprite.setPosition(m_position); // 如果设置了原点，可能需要重新设置位置以匹配逻辑上的startPosition

    std::cout << "Bullet created. Type: " << m_type
              << ", Pos: (" << m_position.x << "," << m_position.y << ")"
              << ", FlyDir: (" << m_flyDirection.x << "," << m_flyDirection.y << ")"
              << ", TankEnumDir: " << static_cast<int>(m_tankDirection) << std::endl;
}

// update, draw, isAlive, setIsAlive, getPosition, getDamage, getType, getBounds 保持和我上次建议的类似

void Bullet::update(sf::Time dt) {
    if (m_isAlive) {
        float dtSeconds = dt.asSeconds();
        m_position += m_flyDirection * m_speed * dtSeconds;
        m_sprite.setPosition(m_position);
    }
}

void Bullet::draw(sf::RenderWindow &window) const {
    if (m_isAlive) {
        window.draw(m_sprite);
    }
}

bool Bullet::isAlive() const {
    return m_isAlive;
}

void Bullet::setIsAlive(bool alive) {
    m_isAlive = alive;
}

sf::Vector2f Bullet::getPosition() const {
    return m_position;
}

int Bullet::getDamage() const {
    return m_damage;
}

int Bullet::getType() const {
    return m_type;
}

// 新增一个 getter (如果需要的话)
Direction Bullet::getTankDirection() const {
    return m_tankDirection;
}

sf::FloatRect Bullet::getBounds() const {
    if (m_isAlive) {
        return m_sprite.getGlobalBounds();
    }
    return sf::FloatRect();
}