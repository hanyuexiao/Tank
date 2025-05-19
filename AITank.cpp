// AITank.cpp
// Created by admin on 2025/5/12.
//

#include "AITank.h"
#include "Bullet.h"     // 如果 AITank 的特定逻辑直接创建子弹 (目前是调用基类 shoot)
#include "Game.h"       // 包含 Game.h 以便使用 Game& game 引用
#include <iostream>     // 用于调试输出
#include <vector>       // 用于 std::vector (例如在 decideNextAction 中)
#include <cmath>        // 用于 std::abs 等数学函数

// MODIFIED: 构造函数实现，增加了 tankType 和 game 参数
float AITank::getRandomValue(float base, float factor) {
    std::uniform_real_distribution<float> dist(-factor, factor);
    return base * (1.0f + dist(m_rng)); // m_rng 是 AITank.h 中已有的随机数引擎
}

int AITank::getRandomValueInt(int base, float factor) {
    std::uniform_real_distribution<float> dist(-factor, factor);
    return static_cast<int>(static_cast<float>(base) * (1.0f + dist(m_rng)));
}


// 修改后的构造函数
AITank::AITank(sf::Vector2f startPosition, Direction direction, const std::string& tankType, Game& game,
               float baseSpeed, int baseHealth, int baseAttack,int frameW,int frameH) // ***接收基础属性***
        : Tank(startPosition, direction, tankType, game,
               1.0f, // 临时速度，会被下面随机化后的值覆盖
               frameW,
               frameH,
               1,    // 临时生命值，会被下面随机化后的值覆盖
               0     /*armor, 默认AI坦克护甲为0*/) ,
          m_isMovingToNextTile(false),
          m_hasStrategicTarget(false),
          m_aiShootTimer(sf::Time::Zero),
        // m_rng 在基类 Tank 的某个地方或者这里如果还没有则需要初始化: m_rng(std::random_device{}())
        // 从 AITank.h 移除 m_rng 的声明，因为它已经在 Tank.h 中有了 m_rng(std::random_device{}())
        // 哦，AITank.h 中的 m_rng 是独立的，用于AI决策的，Tank中没有。所以 AITank 中 m_rng 初始化保留。
          m_cooldownDistribution(0.8f, 2.5f),
          m_baseSpeedForDebuff(1.0f), // 会被下面的随机速度覆盖
          m_wasOriginalDistStored(false),
          m_slowDebuffDuration(sf::Time::Zero),
          m_isSlowDebuffActive(false)
{
    // --- 属性随机化 ---
    // 1. 生命值 (m_MaxHealth, m_health)
    m_MaxHealth = std::max(10, getRandomValueInt(baseHealth, HEALTH_RANDOM_FACTOR)); // 保证 최소 10 HP
    m_health = m_MaxHealth;

    // 2. 速度 (m_baseSpeed, m_speed)
    // Tank 基类的构造函数已经接收了 speed 参数并设置了 m_baseSpeed 和 m_speed。
    // 我们需要覆盖它。
    float randomizedBaseSpeed = std::max(10.0f, getRandomValue(baseSpeed, SPEED_RANDOM_FACTOR)); // 保证 최소 10 速度
    Tank::setSpeed(randomizedBaseSpeed); // 使用我们之前讨论过的 setSpeed 来正确设置 m_baseSpeed, m_originalSpeed, m_speed
    m_baseSpeedForDebuff = randomizedBaseSpeed; // 更新用于debuff的基础速度记录

    // 3. 攻击力 (m_baseAttackPower, m_currentAttackPower)
    m_baseAttackPower = std::max(5, getRandomValueInt(baseAttack, ATTACK_RANDOM_FACTOR)); // 保证 최소 5 攻击力
    m_currentAttackPower = m_baseAttackPower;

    // 确保 m_originalSpeed (在Tank类中) 也被正确设置，如果 setSpeed 没有处理它的话。
    // Tank::setSpeed 的推荐实现是更新 m_baseSpeed 和 m_originalSpeed
    // this->m_originalSpeed = this->m_baseSpeed; // 如果 Tank::setSpeed 没做这个

    std::cout << "AITank (type '" << getTankType() << "') created with randomized stats. "
              << "HP: " << m_health << "/" << m_MaxHealth
              << ", Speed: " << m_speed // 或者 m_baseSpeed，取决于 setSpeed 的具体实现
              << ", Attack: " << m_currentAttackPower << std::endl;

    generateNewRandomCooldown();
}
// 生成一个新的随机射击冷却时间
void AITank::generateNewRandomCooldown() {
    m_aiShootCooldown = sf::seconds(m_cooldownDistribution(m_rng)); // 从分布中抽取一个随机值作为冷却时间
    // std::cout << "AITank (type '" << getTankType() << "') new shoot cooldown generated: " << m_aiShootCooldown.asSeconds() << " seconds" << std::endl;
}

// 检查AI坦克是否可以射击
bool AITank::canShootAI() const {
    return m_aiShootTimer >= m_aiShootCooldown; // 如果计时器超过或等于冷却时间，则可以射击
}

// 重置AI坦克的射击计时器 (通常在射击后调用)
void AITank::resetShootTimerAI() {
    m_aiShootTimer = sf::Time::Zero; // 重置计时器
    generateNewRandomCooldown();     // 生成下一次射击的冷却时间
}

// MODIFIED: AI坦克的更新逻辑，现在接收 Game& game 参数
void AITank::update(sf::Time dt, Game& game) {
    Tank::update(dt, game); // 调用基类 Tank 的 update 方法，传递 game 引用 (用于动画和通用buff等)

    // 更新AI特有的射击计时器
    if(m_aiShootTimer < m_aiShootCooldown) {
        m_aiShootTimer += dt;
    }

    // 处理减速debuff的倒计时逻辑
    if (m_isSlowDebuffActive) {
        m_slowDebuffDuration -= dt; // 减去本帧经过的时间
        if (m_slowDebuffDuration <= sf::Time::Zero) { // 如果debuff时间结束
            // Debuff 结束，恢复原始状态
            setSpeed(m_baseSpeedForDebuff); // 恢复速度 (注意：Tank::setSpeed 可能需要调整以正确处理基础速度和buff速度)

            // 恢复原始的冷却时间分布
            if(m_wasOriginalDistStored){
                m_cooldownDistribution = m_originalCooldownDistribution;
                m_wasOriginalDistStored = false; // 重置标记
            }
            resetShootTimerAI(); // 让原始冷却分布生效

            m_isSlowDebuffActive = false;        // 标记debuff为非激活
            m_slowDebuffDuration = sf::Time::Zero; // 重置debuff持续时间
            std::cout << "AITank (type '" << getTankType() << "') at (" << get_position().x << "," << get_position().y
                      << ") slow debuff expired. Stats restored." << std::endl;
        }
    }
    // 注意: AI的移动决策 (decideNextAction) 和格子间移动 (updateMovementBetweenTiles)
    // 通常在 Game::update 循环中被显式调用，而不是在这个 AITank::update 内部。
    // 这个 AITank::update 主要负责调用基类 update 和处理 AITank 特有的计时器/状态。
}

// 获取AI坦克当前所在的瓦片坐标
sf::Vector2i AITank::getCurrentTile(const Map& map) const {
    if (map.getTileWidth() == 0 || map.getTileHeight() == 0) { // 防止除以零错误
        std::cerr << "AITank::getCurrentTile Error: Map tile dimensions are zero." << std::endl;
        return {-1, -1};
    }
    return {
            static_cast<int>(m_position.x / map.getTileWidth()),
            static_cast<int>(m_position.y / map.getTileHeight())
    };
}

// 获取指定瓦片坐标的像素中心位置
sf::Vector2f AITank::getPixelCenterForTile(int tileX, int tileY, const Map& map) {
    return {
            static_cast<float>(tileX * map.getTileWidth()) + map.getTileWidth() / 2.0f,
            static_cast<float>(tileY * map.getTileHeight()) + map.getTileHeight() / 2.0f
    };
}

// AI决策下一步行动 (例如，向哪个邻近格子移动)
void AITank::decideNextAction(const Map& map, const Tank* playerTankRef) {
    if (m_isMovingToNextTile || isDestroyed()) { // 如果正在移动或已被摧毁，则不进行新的移动决策
        return;
    }

    if (!m_hasStrategicTarget) { // 如果没有战略目标
        // std::cout << "AITank (type '" << getTankType() << "'): No strategic target. Idling or random move." << std::endl;
        // 在此可以实现巡逻或随机移动逻辑
        // 简单示例：尝试随机选择一个可移动方向
        std::vector<Direction> possibleMoves;
        sf::Vector2i currentTile = getCurrentTile(map);
        if (map.isTileWalkable(currentTile.x, currentTile.y - 1)) possibleMoves.push_back(Direction::UP);
        if (map.isTileWalkable(currentTile.x, currentTile.y + 1)) possibleMoves.push_back(Direction::DOWN);
        if (map.isTileWalkable(currentTile.x - 1, currentTile.y)) possibleMoves.push_back(Direction::LEFT);
        if (map.isTileWalkable(currentTile.x + 1, currentTile.y)) possibleMoves.push_back(Direction::RIGHT);

        if (!possibleMoves.empty()) {
            std::uniform_int_distribution<size_t> dist(0, possibleMoves.size() - 1);
            m_intendedDirectionForTileMove = possibleMoves[dist(m_rng)];
        } else {
            // 没有可移动方向，保持当前方向或原地不动
            m_intendedDirectionForTileMove = get_Direction(); // 保持当前方向
            // std::cout << "AITank (type '" << getTankType() << "'): Stuck, no possible random moves." << std::endl;
            return; // 没有可移动方向，直接返回
        }
        // return; // 如果没有战略目标，暂时只做随机方向选择，不立即移动
    } else { // 如果有战略目标
        sf::Vector2i currentTile = getCurrentTile(map);

        // 如果已在目标瓦片 (例如基地)
        if (currentTile.x == m_strategicTargetTileCoordinate.x && currentTile.y == m_strategicTargetTileCoordinate.y) {
            // std::cout << "AITank (type '" << getTankType() << "'): Reached strategic target tile. Considering attack." << std::endl;
            // TODO: 在此加入攻击逻辑，例如调整方向对准基地并射击
            // 简单示例：尝试朝向玩家（如果玩家存在且在附近）或一个固定方向
            if (playerTankRef && !playerTankRef->isDestroyed()) {
                sf::Vector2f dirToPlayer = playerTankRef->get_position() - get_position();
                if (std::abs(dirToPlayer.x) > std::abs(dirToPlayer.y)) {
                    m_intendedDirectionForTileMove = (dirToPlayer.x > 0) ? Direction::RIGHT : Direction::LEFT;
                } else {
                    m_intendedDirectionForTileMove = (dirToPlayer.y > 0) ? Direction::DOWN : Direction::UP;
                }
            } else { // 否则，随机选择一个方向或保持当前方向
                m_intendedDirectionForTileMove = get_Direction(); // 简单保持
            }
            // return; // 到达目标后，暂时不主动移动，等待射击逻辑或下一轮决策
        } else { // 如果未到达战略目标，进行寻路
            // 简化的寻路逻辑：优先尝试直接朝目标瓦片移动
            // 优先级：Y轴 -> X轴 (或可改为更复杂的A*算法)
            bool movedY = false;
            if (m_strategicTargetTileCoordinate.y < currentTile.y && map.isTileWalkable(currentTile.x, currentTile.y - 1)) {
                m_intendedDirectionForTileMove = Direction::UP;
                movedY = true;
            } else if (m_strategicTargetTileCoordinate.y > currentTile.y && map.isTileWalkable(currentTile.x, currentTile.y + 1)) {
                m_intendedDirectionForTileMove = Direction::DOWN;
                movedY = true;
            }

            if (!movedY) { // 如果Y轴无法移动或已对齐，尝试X轴
                if (m_strategicTargetTileCoordinate.x < currentTile.x && map.isTileWalkable(currentTile.x - 1, currentTile.y)) {
                    m_intendedDirectionForTileMove = Direction::LEFT;
                } else if (m_strategicTargetTileCoordinate.x > currentTile.x && map.isTileWalkable(currentTile.x + 1, currentTile.y)) {
                    m_intendedDirectionForTileMove = Direction::RIGHT;
                } else { // X轴也无法直接移动，尝试寻找其他可用路径 (非常基础的“绕行”尝试)
                    std::vector<Direction> possibleMoves;
                    if (map.isTileWalkable(currentTile.x, currentTile.y - 1) && get_Direction() != Direction::DOWN) possibleMoves.push_back(Direction::UP);
                    if (map.isTileWalkable(currentTile.x, currentTile.y + 1) && get_Direction() != Direction::UP) possibleMoves.push_back(Direction::DOWN);
                    if (map.isTileWalkable(currentTile.x - 1, currentTile.y) && get_Direction() != Direction::RIGHT) possibleMoves.push_back(Direction::LEFT);
                    if (map.isTileWalkable(currentTile.x + 1, currentTile.y) && get_Direction() != Direction::LEFT) possibleMoves.push_back(Direction::RIGHT);

                    if (!possibleMoves.empty()) {
                        std::uniform_int_distribution<size_t> dist(0, possibleMoves.size() - 1);
                        m_intendedDirectionForTileMove = possibleMoves[dist(m_rng)];
                    } else {
                        // std::cout << "AITank (type '" << getTankType() << "'): Stuck while pathfinding to strategic target." << std::endl;
                        return; // 没有可移动方向
                    }
                }
            }
        }
    }


    // 根据选定的 m_intendedDirectionForTileMove，设置移动到下一个瓦片的目标
    sf::Vector2i currentTile = getCurrentTile(map);
    sf::Vector2i nextTileToMoveTo = currentTile;
    bool validMoveSelected = true;

    switch (m_intendedDirectionForTileMove) {
        case Direction::UP:    nextTileToMoveTo.y--; break;
        case Direction::DOWN:  nextTileToMoveTo.y++; break;
        case Direction::LEFT:  nextTileToMoveTo.x--; break;
        case Direction::RIGHT: nextTileToMoveTo.x++; break;
        default: validMoveSelected = false; break; // 不应该发生
    }

    // 在实际开始移动前，最后检查一次目标瓦片是否可通行
    if (validMoveSelected && map.isTileWalkable(nextTileToMoveTo.x, nextTileToMoveTo.y)) {
        // Tank::setDirection 方法现在需要 Game& game 参数，但 decideNextAction 没有这个参数。
        // 方向的纹理更新应该在 Tank::setDirection 中处理，而 Tank::setDirection 会在 Game::Handling_events (玩家)
        // 或 AITank::updateMovementBetweenTiles (AI) 中被调用，这些地方可以传递 Game&。
        // 这里只设置逻辑方向 m_direction。实际的 setDirection(dir, game) 调用在 updateMovementBetweenTiles。
        // 或者，如果AI坦克在决策时就需要立即改变外观方向，decideNextAction也需要Game&。
        // 目前保持简单，视觉方向在移动时更新。
        m_direction = m_intendedDirectionForTileMove; // 更新逻辑方向
        m_pixelTargetForTileMove = getPixelCenterForTile(nextTileToMoveTo.x, nextTileToMoveTo.y, map);
        m_isMovingToNextTile = true; // 开始格子间移动

        // std::cout << "AITank (type '" << getTankType() << "') decided to move " << static_cast<int>(m_intendedDirectionForTileMove)
        //           << " towards tile (" << nextTileToMoveTo.x << "," << nextTileToMoveTo.y << ")" << std::endl;
    } else {
        // std::cout << "AITank (type '" << getTankType() << "'): Chosen move to (" << nextTileToMoveTo.x << "," << nextTileToMoveTo.y << ") is not walkable or invalid." << std::endl;
        m_isMovingToNextTile = false; // 无法移动
    }
}

// 更新AI坦克在格子间的平滑移动
void AITank::updateMovementBetweenTiles(sf::Time dt, const Map& map) {
    if (!m_isMovingToNextTile || isDestroyed()) { // 如果不在移动或已被摧毁，则返回
        return;
    }

    // 确保坦克的视觉方向与意图移动方向一致
    // Tank::setDirection 现在需要 Game& game 参数。
    // 这意味着 updateMovementBetweenTiles 也需要 Game& game 参数，或者 Game::update 在调用它时传递。
    // 假设 Game::update 会在调用此方法时传递 Game 引用，或者此方法签名已更新。
    // 为了编译通过，暂时注释掉需要 Game 引用的 setDirection 调用。
    // 你需要在调用此方法的地方（可能是 Game::update）确保 Game 引用被传递，
    // 或者修改 Tank::setDirection 以便在没有 Game 引用的情况下也能设置逻辑方向（如果纹理更新分离）。
    // if (get_Direction() != m_intendedDirectionForTileMove) {
    //    setDirection(m_intendedDirectionForTileMove, /* game_ref_needed_here */);
    // }
    // 临时的简化：直接设置逻辑方向，视觉更新依赖于 Tank::update(dt, game) 中的动画逻辑
    if (m_direction != m_intendedDirectionForTileMove) {
        m_direction = m_intendedDirectionForTileMove; // 设置逻辑方向
        m_currentFrame = 0; // 重置动画帧，让Tank::update去取新方向的纹理
    }


    sf::Vector2f currentPos = get_position();
    float moveAmountThisFrame = getSpeed() * dt.asSeconds(); // 使用 getSpeed() 获取当前速度
    sf::Vector2f desiredNextPixelPos = currentPos;
    bool reachedTargetAxis = false;

    // 根据意图移动方向，计算期望的下一像素位置
    switch (m_intendedDirectionForTileMove) {
        case Direction::UP:
            desiredNextPixelPos.y -= moveAmountThisFrame;
            if (desiredNextPixelPos.y <= m_pixelTargetForTileMove.y) { desiredNextPixelPos.y = m_pixelTargetForTileMove.y; reachedTargetAxis = true; }
            desiredNextPixelPos.x = m_pixelTargetForTileMove.x; // 强制X轴对齐
            break;
        case Direction::DOWN:
            desiredNextPixelPos.y += moveAmountThisFrame;
            if (desiredNextPixelPos.y >= m_pixelTargetForTileMove.y) { desiredNextPixelPos.y = m_pixelTargetForTileMove.y; reachedTargetAxis = true; }
            desiredNextPixelPos.x = m_pixelTargetForTileMove.x; // 强制X轴对齐
            break;
        case Direction::LEFT:
            desiredNextPixelPos.x -= moveAmountThisFrame;
            if (desiredNextPixelPos.x <= m_pixelTargetForTileMove.x) { desiredNextPixelPos.x = m_pixelTargetForTileMove.x; reachedTargetAxis = true; }
            desiredNextPixelPos.y = m_pixelTargetForTileMove.y; // 强制Y轴对齐
            break;
        case Direction::RIGHT:
            desiredNextPixelPos.x += moveAmountThisFrame;
            if (desiredNextPixelPos.x >= m_pixelTargetForTileMove.x) { desiredNextPixelPos.x = m_pixelTargetForTileMove.x; reachedTargetAxis = true; }
            desiredNextPixelPos.y = m_pixelTargetForTileMove.y; // 强制Y轴对齐
            break;
    }

    // 调用基类的 move 方法尝试移动，它会进行地图碰撞检测
    Tank::move(desiredNextPixelPos, map);

    sf::Vector2f actualNewPos = get_position(); // 获取移动后的实际位置

    // 判断是否到达目标格子的中心 (使用小阈值)
    float arrivalThreshold = 1.0f; // 像素阈值可以调小一些，因为我们强制对齐另一轴
    bool alignedX = std::abs(actualNewPos.x - m_pixelTargetForTileMove.x) < arrivalThreshold;
    bool alignedY = std::abs(actualNewPos.y - m_pixelTargetForTileMove.y) < arrivalThreshold;

    if (reachedTargetAxis && alignedX && alignedY) { // 如果逻辑上到达目标轴，并且双轴都已对齐
        m_position = m_pixelTargetForTileMove;    // 精确设置到格子中心
        m_sprite.setPosition(m_position);         // 更新精灵位置
        m_isMovingToNextTile = false;             // 标记格子间移动完成
        // std::cout << "AITank (type '" << getTankType() << "') reached center of target tile." << std::endl;
    } else if (actualNewPos == currentPos && !reachedTargetAxis && sf::Keyboard::isKeyPressed(sf::Keyboard::P)) { // 如果移动受阻 (位置未变) 且未到达目标轴
        // (sf::Keyboard::isKeyPressed(sf::Keyboard::P) 是一个调试条件，可以移除)
        m_isMovingToNextTile = false; // 停止当前移动尝试，让AI重新决策
        // std::cout << "AITank (type '" << getTankType() << "') movement blocked before reaching target axis point." << std::endl;
    }
}

// 设置AI坦克的战略目标瓦片
void AITank::setStrategicTargetTile(sf::Vector2i targetTile) {
    m_strategicTargetTileCoordinate = targetTile;
    m_hasStrategicTarget = (targetTile.x != -1 && targetTile.y != -1); // (-1,-1) 通常表示无效或未设置
    if (m_hasStrategicTarget) {
        // std::cout << "AITank (type '" << getTankType() << "'): Strategic target tile set to (" << targetTile.x << ", " << targetTile.y << ")" << std::endl;
        m_isMovingToNextTile = false; // 当战略目标更新时，重置当前移动状态，以便重新决策
    } else {
        // std::cout << "AITank (type '" << getTankType() << "'): Strategic target cleared or invalid." << std::endl;
    }
}

// 激活AI坦克的减速debuff
void AITank::activateSlowDebuff(float speedMultiplier, float attackSpeedFactor, sf::Time duration) {
    if (!m_isSlowDebuffActive) { // 如果debuff尚未激活
        // 首次激活debuff，存储原始状态
        m_baseSpeedForDebuff = getSpeed(); // 存储当前速度作为debuff前的基础速度

        if (!m_wasOriginalDistStored) { // 确保只存储一次最原始的冷却分布
            m_originalCooldownDistribution = m_cooldownDistribution;
            m_wasOriginalDistStored = true;
        }
    }

    // 应用速度减益 (speedMultiplier 通常 < 1, 例如 0.3f)
    setSpeed(m_baseSpeedForDebuff * speedMultiplier);

    // 应用攻速减益 (调整冷却分布，attackSpeedFactor 通常 > 1, 例如 2.0f 表示冷却时间变为2倍)
    float currentMin = m_originalCooldownDistribution.min();
    float currentMax = m_originalCooldownDistribution.max();
    m_cooldownDistribution = std::uniform_real_distribution<float>(currentMin * attackSpeedFactor, currentMax * attackSpeedFactor);
    resetShootTimerAI(); // 让新的冷却分布立即生效

    m_slowDebuffDuration = duration;    // 设置debuff持续时间
    m_isSlowDebuffActive = true;        // 标记debuff为激活状态

    std::cout << "AITank (type '" << getTankType() << "') SLOWED DOWN. Speed: " << getSpeed()
              << ", Cooldown dist altered. Duration: " << duration.asSeconds() << "s" << std::endl;
}


