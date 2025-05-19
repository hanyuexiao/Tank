#ifndef AITANK_H
#define AITANK_H

// =========================================================================
// 必要的头文件包含
// =========================================================================
#include "Tank.h"       // 包含基类 Tank 的定义
#include "Map.h"        // 包含 Map 类的定义 (用于 AI 决策和移动)
#include <random>       // 用于随机数生成 (例如 m_rng)
#include <string>       // 用于 std::string (作为 tankType)
// Game.h 通常不在 AITank.h 中直接包含，以避免循环依赖，
// Tank 基类的方法签名已经改变，需要 Game&，因此构造函数和update会接收它。

// =========================================================================
// 前向声明 (Forward Declarations)
// =========================================================================
class Game;             // Game 类，AITank 的行为可能需要与 Game 对象交互

class AITank : public Tank {
public:
    // =========================================================================
    // 构造函数
    // =========================================================================
    // 参数:
    //   startPosition - AI坦克的初始位置
    //   direction - AI坦克的初始方向
    //   tankType - AI坦克的类型字符串 (例如 "ai_default", "ai_fast")
    //   game - 对Game对象的引用 (用于基类构造和潜在的AI特定逻辑)
    //   baseSpeed - AI坦克的基础速度
    //   baseHealth - AI坦克的基础生命值
    //   baseAttack - AI坦克的基础攻击力
    //   frameW - 纹理帧宽度
    //   frameH - 纹理帧高度
    //   scoreValue - 击毁此AI坦克获得的分数
    AITank(sf::Vector2f startPosition,
           Direction direction,
           const std::string& tankType,
           Game& game,
           float baseSpeed,
           int baseHealth,
           int baseAttack,
           int frameW,
           int frameH,
           int scoreValue);

    // =========================================================================
    // 核心AI逻辑方法 (由Game类在主循环中调用)
    // =========================================================================
    // AI决策：决定下一步要朝哪个方向移动（或者是否射击等）
    void decideNextAction(const Map& map, const Tank* playerTankRef);

    // 每帧调用，处理正在进行的格子间平滑移动
    void updateMovementBetweenTiles(sf::Time dt, const Map& map);

    // 覆盖基类的 update 方法，处理AI特有的更新逻辑 (如射击计时器、debuff)
    void update(sf::Time dt, Game& game) override;

    // =========================================================================
    // AI状态查询与控制
    // =========================================================================
    // 设置 AI 坦克的战略目标瓦片坐标 (例如，基地位置)
    void setStrategicTargetTile(sf::Vector2i targetTile);

    // 检查 AI 坦克当前是否正在进行格子间的移动
    bool isMoving() const { return m_isMovingToNextTile; }

    // 检查 AI 坦克是否可以射击 (基于其特定的冷却计时器)
    bool canShootAI() const;

    // 重置 AI 坦克的射击计时器并生成新的随机冷却时间
    void resetShootTimerAI();

    // 激活 AI 坦克的减速debuff效果
    void activateSlowDebuff(float speedMultiplier, float attackSpeedFactor, sf::Time duration);

private:
    // =========================================================================
    // AI 特有射击相关成员
    // =========================================================================
    sf::Time m_aiShootCooldown;         // AI 射击的冷却时间 (由 generateNewRandomCooldown 设置)
    sf::Time m_aiShootTimer;            // AI 自上次射击以来经过的时间

    // =========================================================================
    // AI 随机行为与决策支持成员
    // =========================================================================
    std::mt19937 m_rng;                 // Mersenne Twister 随机数引擎
    std::uniform_real_distribution<float> m_cooldownDistribution; // AI射击冷却时间的均匀分布

    // =========================================================================
    // AI 移动状态相关成员
    // =========================================================================
    bool m_isMovingToNextTile;              // 标记坦克当前是否正在从一个格子移动到下一个格子
    sf::Vector2f m_pixelTargetForTileMove;  // 当前格子间移动的目标像素位置 (下一个格子的中心)
    Direction m_intendedDirectionForTileMove; // 本次格子移动的预定方向

    // =========================================================================
    // AI 战略目标相关成员
    // =========================================================================
    sf::Vector2i m_strategicTargetTileCoordinate; // AI 的长期战略目标瓦片坐标
    bool m_hasStrategicTarget;                    // 标记 AI 是否有一个有效的战略目标

    // =========================================================================
    // AI Debuff 相关状态存储
    // =========================================================================
    float m_baseSpeedForDebuff;         // 存储应用 debuff 前的基础速度 (用于恢复)
    std::uniform_real_distribution<float> m_originalCooldownDistribution; // 存储原始的射击冷却分布 (用于恢复)
    bool m_wasOriginalDistStored;       // 标记原始冷却分布是否已存储
    sf::Time m_slowDebuffDuration;      // 减速 debuff 的剩余持续时间
    bool m_isSlowDebuffActive;          // 标记减速 debuff 是否激活

    // =========================================================================
    // AI 属性随机化相关常量
    // =========================================================================
    static constexpr float HEALTH_RANDOM_FACTOR = 0.2f;  // 生命值上下浮动20%
    static constexpr float SPEED_RANDOM_FACTOR = 0.15f; // 速度上下浮动15%
    static constexpr float ATTACK_RANDOM_FACTOR = 0.25f; // 攻击力上下浮动25%

    // =========================================================================
    // 私有辅助方法
    // =========================================================================
    // 生成一个新的随机射击冷却时间
    void generateNewRandomCooldown();

    // 获取指定瓦片坐标的像素中心位置
    static sf::Vector2f getPixelCenterForTile(int tileX, int tileY, const Map& map);

    // 获取 AI 坦克当前所在的瓦片坐标
    sf::Vector2i getCurrentTile(const Map& map) const;

    // 辅助方法，用于在一个范围内生成随机值 (用于属性随机化)
    float getRandomValue(float base, float factor);
    int getRandomValueInt(int base, float factor);
};

#endif //AITANK_H
