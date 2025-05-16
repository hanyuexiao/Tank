// AITank.h
// Created by admin on 2025/5/12.
//

#ifndef AITANK_H
#define AITANK_H

#include <random>       // 用于随机数生成 (例如 m_rng)
#include <string>       // 用于 std::string (作为 tankType)
#include "Tank.h"       // 包含基类 Tank 的定义
#include "Map.h"        // 包含 Map 类的定义 (用于 AI 决策和移动)
// #include "Game.h"    // Game.h 通常不在 AITank.h 中直接包含，而是使用前向声明
// 并在 AITank.cpp 中包含，以避免循环依赖。
// 但如果 Game 类的方法被内联调用或其类型作为成员，则可能需要。
// 这里 Tank 基类的方法签名已经改变，需要 Game&。

// 前向声明 Game 类，因为 Tank 基类的 update 和构造函数等现在需要 Game 引用
class Game;

class AITank : public Tank {
public:
    // MODIFIED: 构造函数签名已更改
    // 参数:
    //   startPosition - AI 坦克的初始位置
    //   direction - AI 坦克的初始方向
    //   tankType - AI 坦克的类型字符串 (例如 "ai_default", "ai_heavy")，用于从 Game 获取纹理
    //   game - 对 Game 对象的引用，用于获取纹理和其他游戏上下文
    //   speed - AI 坦克的移动速度
    //   frameWidth - 坦克纹理单帧宽度
    //   frameHeight - 坦克纹理单帧高度
    //   inihp - 初始生命值
    AITank(sf::Vector2f startPosition,
           Direction direction,
           const std::string& tankType, // 新增：坦克类型，用于纹理查找
           Game& game,                  // 新增：Game 对象的引用
           float speed = 80.f,
           int frameWidth = 50,
           int frameHeight = 50,
           int inihp = 100);

    // AI 决策：决定下一步要朝哪个方向移动（或者是否射击等）
    void decideNextAction(const Map& map, const Tank* playerTankRef);

    // 设置 AI 坦克的战略目标瓦片坐标 (例如，基地位置)
    void setStrategicTargetTile(sf::Vector2i targetTile);

    // 每帧调用，处理正在进行的格子间平滑移动
    void updateMovementBetweenTiles(sf::Time dt, const Map& map);

    // 检查 AI 坦克当前是否正在进行格子间的移动
    bool isMoving() const { return m_isMovingToNextTile; }

    // 检查 AI 坦克是否可以射击 (基于其特定的冷却计时器)
    bool canShootAI() const;

    // 重置 AI 坦克的射击计时器并生成新的随机冷却时间
    void resetShootTimerAI();

    // MODIFIED: 覆盖基类的 update 方法，现在需要 Game& game 参数
    void update(sf::Time dt, Game& game) override;

    // 激活 AI 坦克的减速debuff效果
    void activateSlowDebuff(float speedMultiplier, float attackSpeedFactor, sf::Time duration);

private:
    // AI 特有的射击冷却相关成员
    sf::Time m_aiShootCooldown; // AI 射击的冷却时间 (由 generateNewRandomCooldown 设置)
    sf::Time m_aiShootTimer;    // AI 自上次射击以来经过的时间

    // 随机数生成器，用于射击冷却等随机行为
    std::mt19937 m_rng; // Mersenne Twister 随机数引擎
    std::uniform_real_distribution<float> m_cooldownDistribution; // 射击冷却时间的均匀分布

    // 生成一个新的随机射击冷却时间
    void generateNewRandomCooldown();

    // AI 移动状态相关成员
    bool m_isMovingToNextTile;              // 标记坦克当前是否正在从一个格子移动到下一个格子
    sf::Vector2f m_pixelTargetForTileMove;  // 当前格子间移动的目标像素位置 (下一个格子的中心)
    Direction m_intendedDirectionForTileMove; // 本次格子移动的预定方向

    // AI 战略目标相关成员
    sf::Vector2i m_strategicTargetTileCoordinate; // AI 的长期战略目标瓦片坐标
    bool m_hasStrategicTarget;                    // 标记 AI 是否有一个有效的战略目标

    // 辅助函数：获取指定瓦片坐标的像素中心位置
    static sf::Vector2f getPixelCenterForTile(int tileX, int tileY, const Map& map) ;
    // 辅助函数：获取 AI 坦克当前所在的瓦片坐标
    sf::Vector2i getCurrentTile(const Map& map) const;

    // Debuff 相关状态存储
    float m_baseSpeedForDebuff; // 存储应用 debuff 前的基础速度
    std::uniform_real_distribution<float> m_originalCooldownDistribution; // 存储原始的射击冷却分布
    bool m_wasOriginalDistStored; // 标记原始冷却分布是否已存储

    sf::Time m_slowDebuffDuration; // 减速 debuff 的剩余持续时间
    bool m_isSlowDebuffActive;     // 标记减速 debuff 是否激活
};

#endif //AITANK_H
