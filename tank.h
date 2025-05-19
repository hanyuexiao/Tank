#ifndef TANK_H
#define TANK_H

// =========================================================================
// 必要的头文件包含
// =========================================================================
#include "heads.h"      // 包含项目通用的头文件 (如 SFML/Graphics.hpp, iostream 等)
#include "Map.h"        // 包含地图类定义，用于碰撞检测和移动限制
#include "common.h"     // 包含通用定义，例如 Direction 枚举
#include <vector>       // 使用 std::vector (虽然纹理现在由Game管理，但保留以防其他用途)
#include <string>       // 使用 std::string

// =========================================================================
// 宏定义与常量
// =========================================================================
#define FOREST_SLOWDOWN_FACTOR 0.7f // 森林地形的减速因子

// =========================================================================
// 前向声明 (Forward Declarations)
// =========================================================================
class Bullet;           // 子弹类，Tank 可以发射子弹
class Game;             // 游戏主类，Tank 需要与 Game 对象交互 (例如获取纹理、发射子弹时)

class Tank {
protected:
    // =========================================================================
    // 视觉与动画相关成员
    // =========================================================================
    // std::vector<sf::Texture> m_textures; // 纹理现在由 Game 类管理和提供
    sf::Sprite m_sprite;                 // 坦克的精灵，用于在窗口上绘制
    int m_currentFrame;                  // 当前动画帧索引 (如果每个方向有多帧动画)
    int m_frameWidth;                    // 坦克纹理单帧的宽度 (像素)
    int m_frameHeight;                   // 坦克纹理单帧的高度 (像素)

    // =========================================================================
    // 位置与移动相关成员
    // =========================================================================
    sf::Vector2f m_position;             // 坦克在地图上的精确位置 (通常是中心点)
    Direction m_direction;               // 坦克当前的朝向 (UP, DOWN, LEFT, RIGHT)
    float m_baseSpeed;                   // 坦克的基础移动速度 (不受临时buff或地形影响)
    float m_speed;                       // 坦克当前的实际移动速度 (像素/秒)
    float m_originalSpeed;            // 用于在setSpeed时同步m_baseSpeed，或在地形减速时基于一个不变的“出厂速度”
    // 当前m_baseSpeed已承担此角色，可考虑是否仍需m_originalSpeed
    bool m_isInForest;                   // 标记坦克是否在森林中 (影响速度)

    // =========================================================================
    // 射击相关成员
    // =========================================================================
    sf::Time m_baseShootCooldown;        // 基础射击冷却时间 (不受buff影响)
    sf::Time m_shootCooldown;            // 当前实际射击的最小时间间隔 (冷却时间)
    sf::Time m_shootTimer;               // 自上次射击以来经过的时间，用于计算冷却
    int m_baseAttackPower;               // 基础攻击力
    int m_currentAttackPower;            // 当前实际攻击力 (受buff影响)

    // =========================================================================
    // Buff 与状态相关成员
    // =========================================================================
    // 攻击力 Buff
    sf::Time m_attackBuffDuration;       // 攻击力buff剩余持续时间
    bool m_isAttackBuffActive;           // 标记攻击力buff是否激活

    // 攻击速度 Buff
    sf::Time m_attackSpeedBuffDuration;  // 攻速buff剩余持续时间
    bool m_isAttackSpeedBuffActive;      // 标记攻速buff是否激活

    // 移动速度 Buff
    sf::Time m_movementSpeedBuffDuration; // 移速buff剩余持续时间
    bool m_isMovementSpeedBuffActive;     // 标记移速buff是否激活
    float m_movementSpeedBuffIncrease;    // 移速buff提供的速度增加量

    // =========================================================================
    // 生命、护甲与状态
    // =========================================================================
    int m_health;                        // 坦克当前的生命值
    int m_MaxHealth;                     // 坦克的最大生命值
    int m_armor;                         // 坦克当前的护甲值
    bool m_Destroyed;                    // 标记坦克是否已被摧毁 (true 表示已摧毁)

    // =========================================================================
    // 其他属性
    // =========================================================================
    std::string m_tankType;              // 坦克类型 (例如 "player", "ai_default", "ai_fast")
    int m_scoreValue;                    // 击毁此坦克可获得的分数 (主要用于AI坦克)

    // =========================================================================
    // 静态常量
    // =========================================================================
    static const int MAX_ARMOR = 1;      // 坦克的最大护甲值上限

public:
    // =========================================================================
    // 构造函数与析构函数
    // =========================================================================
    // 参数:
    //   startPosition - 初始位置 (中心点)
    //   startDirection - 初始方向
    //   tankType - 坦克类型字符串 (用于从Game获取纹理等)
    //   game - 对Game对象的引用
    //   speed - 初始基础速度
    //   frameWidth - 纹理帧宽度
    //   frameHeight - 纹理帧高度
    //   iniHealth - 初始生命值
    //   armor - 初始护甲值
    //   scoreValue - 击毁该坦克获得的分数
    Tank(sf::Vector2f startPosition, Direction startDirection, const std::string& tankType, Game& game,
         float speed, int frameWidth, int frameHeight, int iniHealth, int armor, int scoreValue);
    virtual ~Tank() = default;           // 虚析构函数，确保派生类的析构函数被正确调用

    // =========================================================================
    // 核心游戏逻辑方法
    // =========================================================================
    void draw(sf::RenderWindow& window); // 将坦克绘制到指定的渲染窗口
    virtual void update(sf::Time dt, Game& game);    // 更新坦克状态 (动画、计时器、buff等)，dt是帧间隔时间
    void move(sf::Vector2f targetPosition, const Map& map); // 尝试将坦克移动到目标位置，会进行地图碰撞检测
    void shoot(Game& gameInstance);      // 创建并发射一颗子弹 (通过Game对象池)

    // =========================================================================
    // Setter 方法
    // =========================================================================
    void setDirection(Direction dir, Game& game);   // 设置坦克的新方向，并请求Game更新纹理
    void setSpeed(float newSpeed);                  // 设置坦克的基础速度
    void setArmor(int newArmor);                    // 设置坦克的护甲值 (会受MAX_ARMOR限制)

    // =========================================================================
    // Getter 方法
    // =========================================================================
    // 位置与尺寸
    sf::Vector2f get_position() const { return m_position; }
    Direction get_Direction() const { return m_direction; } // 注意：原为 get_Direction()，保持一致或改为 getDirection()
    int getFrameWidth() const { return m_frameWidth; }     // 原为 get_TileWight()
    int getFrameHeight() const { return m_frameHeight; }   // 原为 get_TileHeight()
    sf::FloatRect getBounds() const { return m_sprite.getGlobalBounds(); } // 获取全局边界框

    // 状态与属性
    float getSpeed() const { return m_speed; }
    int getHealth() const { return m_health; }
    int getMaxHealth() const { return m_MaxHealth; }
    int getArmor() const { return m_armor; }
    bool isDestroyed() const { return m_Destroyed; }
    const std::string& getTankType() const { return m_tankType; }
    int getScoreValue() const { return m_scoreValue; }
    int getCurrentAttackPower() const; // 获取当前攻击力 (考虑buff)
    sf::Time getShootCooldown() const { return m_shootCooldown; } // 获取当前射击冷却

    // 射击状态
    bool canShoot() const { return m_shootTimer >= m_shootCooldown; }
    void resetShootTimer() { m_shootTimer = sf::Time::Zero; }

    // =========================================================================
    // Buff激活方法
    // =========================================================================
    void activateAttackBuff(float multiplier, sf::Time duration);
    void activateAttackSpeedBuff(float cooldownMultiplier, sf::Time duration);
    void activateMovementSpeedBuff(float increaseAmount, sf::Time duration);

    // =========================================================================
    // 生命与伤害处理
    // =========================================================================
    void takeDamage(int damageAmount); // 坦克受到伤害
    void revive(sf::Vector2f position, Direction direction, Game& game); // 复活/重置坦克状态
};

#endif // TANK_H
