#ifndef TANKS_BULLET_H
#define TANKS_BULLET_H

// =========================================================================
// 必要的头文件包含
// =========================================================================
#include <SFML/Graphics.hpp> // 包含 SFML 图形库，用于纹理、精灵、向量等
#include <cmath>             // 包含数学函数，例如 sqrt 用于向量归一化
#include "common.h"          // 包含通用定义，例如 Direction 枚举

// =========================================================================
// 辅助函数 (如果只在此文件使用，可以放在匿名命名空间或标记为static inline)
// =========================================================================
// 向量归一化：将给定的二维向量转换为单位向量 (长度为1)。
// 参数: source - 需要归一化的源向量
// 返回: 归一化后的向量；如果源向量为零向量，则返回零向量。
inline sf::Vector2f normalize(const sf::Vector2f& source) {
    float length = std::sqrt((source.x * source.x) + (source.y * source.y));
    if (length != 0.f) { // 检查非零以避免除以零
        return sf::Vector2f(source.x / length, source.y / length);
    }
    return sf::Vector2f(0.f, 0.f); // 如果是零向量，返回零向量
}

// =========================================================================
// Bullet 类定义
// =========================================================================
class Bullet {
public:
    // =========================================================================
    // 构造函数与重置方法 (用于对象池)
    // =========================================================================
    // 参数:
    //   texture - 子弹的纹理 (应已设置为正确的朝向)
    //   startPosition - 子弹的初始世界坐标
    //   tankDirectionEnum - 发射时坦克的方向枚举
    //   flyDirectionVec - 子弹飞行的标准化方向向量
    //   damage - 子弹造成的伤害值
    //   speed - 子弹的飞行速度 (像素/秒)
    //   type - 子弹的类型 (整数，可用于区分玩家/AI子弹等)
    Bullet(const sf::Texture& texture,
           sf::Vector2f startPosition,
           Direction tankDirectionEnum,
           sf::Vector2f flyDirectionVec,
           int damage,
           float speed,
           int type);

    // 重置子弹状态，用于从对象池中复用子弹
    void reset(const sf::Texture& texture,
               sf::Vector2f startPosition,
               Direction tankDirectionEnum,
               sf::Vector2f flyDirectionVec,
               int damage,
               float speed,
               int type);

    // =========================================================================
    // 核心功能方法
    // =========================================================================
    void update(sf::Time dt);            // 更新子弹状态 (主要是移动)，dt 是帧间隔时间。
    void draw(sf::RenderWindow &window) const; // 将子弹绘制到指定的渲染窗口

    // =========================================================================
    // Getter 和 Setter 方法
    // =========================================================================
    bool isAlive() const;               // 检查子弹是否仍然存活/有效
    void setIsAlive(bool alive);        // 设置子弹的存活状态

    sf::Vector2f getPosition() const;   // 获取子弹当前的位置
    sf::FloatRect getBounds() const;    // 获取子弹的全局边界框，用于碰撞检测

    int getDamage() const;              // 获取子弹的伤害值
    int getType() const;                // 获取子弹的类型
    Direction getTankDirection() const; // 获取发射此子弹时坦克的方向枚举
    // sf::Vector2f getFlyDirection() const { return m_flyDirection; } // (可选) 如果外部需要知道飞行向量

private:
    // =========================================================================
    // 私有成员变量 - 视觉表现
    // =========================================================================
    sf::Sprite      m_sprite;           // 子弹的精灵，用于绘制

    // =========================================================================
    // 私有成员变量 - 状态与物理属性
    // =========================================================================
    sf::Vector2f    m_position;         // 子弹当前在世界中的精确位置 (通常是中心点)
    sf::Vector2f    m_flyDirection;     // 子弹飞行的标准化方向向量 (用于移动计算)
    float           m_speed;            // 子弹的飞行速度 (像素/秒)
    bool            m_isAlive;          // 标记子弹是否有效/存活 (用于对象池和逻辑处理)

    // =========================================================================
    // 私有成员变量 - 逻辑属性
    // =========================================================================
    Direction       m_tankDirection;    // 记录发射时坦克的方向枚举 (可能用于特殊逻辑或调试)
    int             m_damage;           // 子弹造成的伤害值
    int             m_type;             // 子弹的类型标识 (例如，区分玩家子弹和AI子弹)
    // Tank* m_owner;                  // (可选) 指向发射此子弹的 Tank 对象，如果需要区分发射者
};

#endif //TANKS_BULLET_H
