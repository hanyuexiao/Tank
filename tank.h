#ifndef TANK_H
#define TANK_H

#include "heads.h"      // 包含项目通用的头文件 (如 SFML/Graphics.hpp, iostream 等)
#include "Map.h"        // 包含地图类定义，用于碰撞检测和移动限制
#include <vector>       // 使用 std::vector 存储纹理
#include <string>       // 使用 std::string (虽然这里没直接用，但常见)
#include "common.h"     // 包含通用定义，例如 Direction 枚举

// 前向声明，避免头文件循环依赖
class Bullet;           // 子弹类，Tank 可以发射子弹
class Game;             // 游戏主类，Tank 可能需要与 Game 对象交互 (例如发射子弹时)

class Tank {

protected:
    // 视觉与动画
    std::vector<sf::Texture> m_textures; // 存储坦克不同方向和动画帧的纹理
    sf::Sprite m_sprite;                 // 坦克的精灵，用于在窗口上绘制
    int m_currentFrame;                  // 当前动画帧索引 (如果每个方向有多帧动画)
    int m_frameWidth;                    // 坦克纹理单帧的宽度 (像素)
    int m_frameHeight;                   // 坦克纹理单帧的高度 (像素)

    // 位置与移动
    sf::Vector2f m_position;             // 坦克在地图上的精确位置 (通常是左上角)
    Direction m_direction;               // 坦克当前的朝向 (UP, DOWN, LEFT, RIGHT)
    float m_speed;                       // 坦克的移动速度 (像素/秒)
    float m_baseSpeed;
    sf::Time m_movementSpeedBuffDuration;
    bool m_isMovementSpeedBuffActive;

    // 射击相关
    sf::Time m_shootCooldown;            // 射击的最小时间间隔 (冷却时间)
    sf::Time m_shootTimer;               // 自上次射击以来经过的时间，用于计算冷却
    int m_baseAttackPower;
    int m_currentAttackPower;
    sf::Time m_attackBuffDuration;
    bool m_isAttackBuffActive;
    sf::Time m_baseShootCooldown;
    sf::Time m_attackSpeedBuffDuration;
    bool m_isAttackSpeedBuffActive;

    // 生命状态
    int m_health;                        // 坦克当前的生命值
    int m_MaxHealth;                     // 坦克的最大生命值
    bool m_Destroyed;                    // 标记坦克是否已被摧毁 (true 表示已摧毁)
    int m_armor;
    static const int MAX_ARMOR = 1;

    // 内部辅助函数
    void loadTextures();                 // 私有方法，用于加载坦克所需的所有纹理

public:
    // 构造函数与析构函数
    // 参数: startPosition - 初始位置, direction - 初始方向, speed - 移动速度,
    //       frameWidth - 帧宽, frameHeight - 帧高, health - 初始生命值, max_health - 最大生命值
    Tank(sf::Vector2f startPosition, Direction direction, float speed = 100.f, int frameWidth = 50, int frameHeight = 50, int initialHealth = 100,int m_arrmor = 0);
    virtual ~Tank() = default;           // 虚析构函数，确保派生类的析构函数被正确调用

    // 核心功能方法
    void draw(sf::RenderWindow& window); // 将坦克绘制到指定的渲染窗口
    virtual void update(sf::Time dt);    // 更新坦克状态 (例如动画、射击冷却计时器等)，dt 是帧间隔时间
    void setDirection(Direction dir);    // 设置坦克的新方向，并更新纹理
    void move(sf::Vector2f targetPosition, const Map& map); //尝试将坦克移动到目标位置，会进行地图碰撞检测
    std::unique_ptr<Bullet> shoot(Game& gameInstance); // 创建并发射一颗子弹，返回子弹对象的智能指针

    // --- Getter 方法 ---
    // 获取坦克当前的位置 (const 版本，确保不修改成员)
    sf::Vector2f get_position() const { return m_position; }
    // 获取坦克的速度
    float getSpeed() const { return m_speed; }
    // 获取坦克当前的方向
    Direction get_Direction() { return m_direction; }
    // 获取坦克精灵的帧宽度 (应与 m_frameWidth 一致)
    int get_TileWight() const { return m_frameWidth; } // 建议统一命名为 getFrameWidth()
    // 获取坦克精灵的帧高度 (应与 m_frameHeight 一致)
    int get_TileHeight() const { return m_frameHeight; } // 建议统一命名为 getFrameHeight()
    // 获取坦克的全局边界框，用于碰撞检测
    sf::FloatRect getBounds() const { return m_sprite.getGlobalBounds(); }
    int getArmor() const { return m_armor; }


    // --- 射击相关 Getter 和 Setter ---
    // 检查坦克是否可以射击 (冷却时间是否已过)
    bool canShoot() const { return m_shootTimer >= m_shootCooldown; }
    // 重置射击计时器 (通常在成功射击后调用)
    void resetShootTimer() { m_shootTimer = sf::Time::Zero; }

    // --- 生命值相关方法 (需要你接下来在 .cpp 文件中实现) ---
     void takeDamage(int damageAmount); // 坦克受到伤害
     int getHealth() const { return m_health; } // 获取当前生命值
     int getMaxHealth() const { return m_MaxHealth; } // 获取最大生命值
     bool isDestroyed() const { return m_Destroyed; } // 判断坦克是否已被摧毁
     void revive(sf::Vector2f position, Direction direction); // 复活/重置坦克状态
     void setArmor(int newArmor);
     void activateAttackBuff(float multiplier,sf::Time duration);
     int getCurrentAttackPower() const;
     void activateAttackSpeedBuff(float cooldownMultiplier,sf::Time duration);

     void setSpeed(float newSpeed);
     void activateMovementSpeedBuff(float increaseAmount,sf::Time duration);
};

#endif // TANK_H