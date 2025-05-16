// Bullet.h
#ifndef TANKS_BULLET_H
#define TANKS_BULLET_H

#include <SFML/Graphics.hpp> // 包含 SFML 图形库，用于纹理、精灵、向量等
#include <cmath>             // 包含数学函数，例如 sqrt 用于向量归一化
#include "common.h"          // 包含通用定义，例如 Direction 枚举

// 前向声明 Tank 类，如果需要追踪子弹的发射者 (owner)
// class Tank;

// 辅助函数：向量归一化
// 将给定的二维向量转换为单位向量 (长度为1)，如果原向量长度不为0。
// 参数: source - 需要归一化的源向量
// 返回: 归一化后的向量；如果源向量为零向量，则返回零向量。
inline sf::Vector2f normalize(const sf::Vector2f& source) {
    float length = std::sqrt((source.x * source.x) + (source.y * source.y));
    if (length != 0.f) { // 检查非零以避免除以零
        return sf::Vector2f(source.x / length, source.y / length);
    }
    return sf::Vector2f(0.f, 0.f); // 如果是零向量，返回零向量
}

class Bullet {
public:
    // 构造函数
    // 参数:
    //   texture - 子弹的纹理 (应已设置为正确的朝向，由调用者提供)
    //   startPosition - 子弹的初始世界坐标 (通常是炮口位置)
    //   tankDirectionEnum - 发射时坦克的方向枚举 (例如 Direction::UP)，可用于记录或特定逻辑
    //   flyDirectionVec - 子弹飞行的标准化方向向量 (例如 (0, -1) 代表向上)
    //   damage - 子弹造成的伤害值
    //   speed - 子弹的飞行速度 (像素/秒)
    //   type - 子弹的类型 (整数，可用于区分不同种类的子弹)
    //   owner - (可选) 指向发射此子弹的 Tank 对象的指针
    Bullet(const sf::Texture& texture,
           sf::Vector2f startPosition,
           Direction tankDirectionEnum, // 来自 common.h 的 Direction
           sf::Vector2f flyDirectionVec,
           int damage,
           float speed,
           int type /*, Tank* owner = nullptr */); // owner 可以是可选参数

    // 核心功能方法
    void update(sf::Time dt);            // 更新子弹状态 (移动等)，dt 是帧间隔时间。
    void draw(sf::RenderWindow &window) const; // 将子弹绘制到指定的渲染窗口

    // --- Getter 和 Setter ---
    bool isAlive() const;               // 检查子弹是否仍然存活/有效
    void setIsAlive(bool alive);        // 设置子弹的存活状态

    sf::Vector2f getPosition() const;   // 获取子弹当前的位置
    int getDamage() const;              // 获取子弹的伤害值
    int getType() const;                // 获取子弹的类型
    Direction getTankDirection() const; // 获取发射此子弹时坦克的方向枚举
    sf::FloatRect getBounds() const;    // 获取子弹的全局边界框，用于碰撞检测
    // Tank* getOwner() const;          // 如果实现了 owner 逻辑

private:
    // 视觉表现
    sf::Sprite      m_sprite;           // 子弹的精灵，用于绘制

    // 状态与物理属性
    sf::Vector2f    m_position;         // 子弹当前在世界中的精确位置
    sf::Vector2f    m_flyDirection;     // 子弹飞行的标准化方向向量 (用于移动计算)
    float           m_speed;            // 子弹的飞行速度 (像素/秒)
    bool            m_isAlive;          // 标记子弹是否有效/存活

    // 逻辑属性
    Direction       m_tankDirection;    // 记录发射时坦克的方向枚举
    int             m_damage;           // 子弹造成的伤害值
    int             m_type;             // 子弹的类型标识
    // Tank* m_owner;         // (可选) 指向发射此子弹的 Tank 对象
};

#endif //TANKS_BULLET_H
