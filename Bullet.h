// Bullet.h
#ifndef TANKS_BULLET_H
#define TANKS_BULLET_H

#include <SFML/Graphics.hpp>
#include <cmath>
#include "common.h"

// 辅助函数：向量标准化 (如果你的 Tank 类或者其他地方还没有类似函数)
inline sf::Vector2f normalize(const sf::Vector2f& source) {
    float length = std::sqrt((source.x * source.x) + (source.y * source.y));
    if (length != 0) {
        return {source.x / length, source.y / length};
    }
    return {0.f, 0.f};
}

class Bullet {
public:
    // 构造函数：
    // texture: 已经是对应发射方向的纹理了
    // startPosition: 子弹的起始世界坐标
    // tankDirectionEnum: 坦克发射时的方向枚举 (例如 Tank::Direction::UP)
    // flyDirectionVec: 根据 tankDirectionEnum 计算出的标准化飞行向量 (例如 (0, -1) for UP)
    Bullet(const sf::Texture& texture,
           sf::Vector2f startPosition,
           Direction tankDirectionEnum, // 用于可能的类型区分或记录
           sf::Vector2f flyDirectionVec,   // 用于实际移动
           int damage,
           float speed,
           int type);


    void update(sf::Time dt);
    void draw(sf::RenderWindow &window) const;

    bool isAlive() const;
    void setIsAlive(bool alive);

    sf::Vector2f getPosition() const;
    int getDamage() const;
    int getType() const;
    Direction getTankDirection() const; // 获取发射时的坦克方向枚举
    sf::FloatRect getBounds() const;

private:
    sf::Sprite      m_sprite;
    sf::Vector2f    m_position;
    sf::Vector2f    m_flyDirection; // 标准化的飞行方向向量 (用于移动)
    Direction m_tankDirection;  // 记录发射时的坦克方向枚举 (用于纹理或逻辑)
    float           m_speed;
    int             m_damage;
    int             m_type;
    bool            m_isAlive;
};

#endif //TANKS_BULLET_H