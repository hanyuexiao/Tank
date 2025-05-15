//
// Created by admin on 2025/5/12.
//

#include "AITank.h"
#include "Bullet.h" // 如果 decideAction 直接创建子弹

AITank::AITank(sf::Vector2f startPosition, Direction direction, float speed, int frameWidth, int frameHeight,int inihp)
        : Tank(startPosition, direction, speed, frameWidth, frameHeight,inihp),
          m_isMovingToNextTile(false), m_hasStrategicTarget(false) {
    std::cout << "AITank (Tile-based) created. Speed: " << m_speed << std::endl;
}

sf::Vector2i AITank::getCurrentTile(const Map& map) const {
    if (map.getTileWidth() == 0 || map.getTileHeight() == 0) return {-1, -1}; // 防止除零
    return {
            static_cast<int>(m_position.x / map.getTileWidth()),
            static_cast<int>(m_position.y / map.getTileHeight())
    };
}

sf::Vector2f AITank::getPixelCenterForTile(int tileX, int tileY, const Map& map) {
    return {
            static_cast<float>(tileX * map.getTileWidth()) + map.getTileWidth() / 2.0f,
            static_cast<float>(tileY * map.getTileHeight()) + map.getTileHeight() / 2.0f
    };
}

// AI决策层：决定下一步的行动（比如，向哪个方向的邻近格子移动）

// AI执行层：平滑地完成从当前格子到目标格子的移动
void AITank::updateMovementBetweenTiles(sf::Time dt, const Map& map) {
    if (!m_isMovingToNextTile) {
        return;
    }

    if (get_Direction() != m_intendedDirectionForTileMove) {
        setDirection(m_intendedDirectionForTileMove);
    }

    sf::Vector2f currentPos = get_position();
    // m_pixelTargetForTileMove 是目标格子的中心，我们用它来判断是否到达，
    // 但移动本身严格按 m_intendedDirectionForTileMove 进行。

    float moveAmountThisFrame = m_speed * dt.asSeconds();
    sf::Vector2f desiredNextPixelPos = currentPos; // 初始化为当前位置

    bool reachedTargetAxis = false; // 标记在该轴上是否已到达或超过目标

    switch (m_intendedDirectionForTileMove) {
        case Direction::UP:
            if (currentPos.y > m_pixelTargetForTileMove.y) { // 还在目标上方 (Y值更大)
                desiredNextPixelPos.y -= moveAmountThisFrame;
                if (desiredNextPixelPos.y <= m_pixelTargetForTileMove.y) {
                    desiredNextPixelPos.y = m_pixelTargetForTileMove.y; // 不超过目标
                    reachedTargetAxis = true;
                }
            } else { // 已经到达或超过
                desiredNextPixelPos.y = m_pixelTargetForTileMove.y;
                reachedTargetAxis = true;
            }
            desiredNextPixelPos.x = m_pixelTargetForTileMove.x; // 强制X轴对齐到目标格子的X中心 (重要!)
            break;
        case Direction::DOWN:
            if (currentPos.y < m_pixelTargetForTileMove.y) { // 还在目标下方 (Y值更小)
                desiredNextPixelPos.y += moveAmountThisFrame;
                if (desiredNextPixelPos.y >= m_pixelTargetForTileMove.y) {
                    desiredNextPixelPos.y = m_pixelTargetForTileMove.y;
                    reachedTargetAxis = true;
                }
            } else {
                desiredNextPixelPos.y = m_pixelTargetForTileMove.y;
                reachedTargetAxis = true;
            }
            desiredNextPixelPos.x = m_pixelTargetForTileMove.x; // 强制X轴对齐
            break;
        case Direction::LEFT:
            if (currentPos.x > m_pixelTargetForTileMove.x) { // 还在目标右方 (X值更大)
                desiredNextPixelPos.x -= moveAmountThisFrame;
                if (desiredNextPixelPos.x <= m_pixelTargetForTileMove.x) {
                    desiredNextPixelPos.x = m_pixelTargetForTileMove.x;
                    reachedTargetAxis = true;
                }
            } else {
                desiredNextPixelPos.x = m_pixelTargetForTileMove.x;
                reachedTargetAxis = true;
            }
            desiredNextPixelPos.y = m_pixelTargetForTileMove.y; // 强制Y轴对齐
            break;
        case Direction::RIGHT:
            if (currentPos.x < m_pixelTargetForTileMove.x) { // 还在目标左方 (X值更小)
                desiredNextPixelPos.x += moveAmountThisFrame;
                if (desiredNextPixelPos.x >= m_pixelTargetForTileMove.x) {
                    desiredNextPixelPos.x = m_pixelTargetForTileMove.x;
                    reachedTargetAxis = true;
                }
            } else {
                desiredNextPixelPos.x = m_pixelTargetForTileMove.x;
                reachedTargetAxis = true;
            }
            desiredNextPixelPos.y = m_pixelTargetForTileMove.y; // 强制Y轴对齐
            break;
    }

    Tank::move(desiredNextPixelPos, map); // 尝试移动

    // 移动后再次获取实际位置，因为 Tank::move 可能因碰撞而没有完全移动到 desiredNextPixelPos
    sf::Vector2f actualNewPos = get_position();

    // 判断是否真正到达了目标格子的中心
    // 使用一个小的阈值来判断是否对齐，因为浮点数比较可能不精确
    float arrivalThreshold = 1.5f; // 像素阈值 (和之前一样)
    bool alignedX = std::abs(actualNewPos.x - m_pixelTargetForTileMove.x) < arrivalThreshold;
    bool alignedY = std::abs(actualNewPos.y - m_pixelTargetForTileMove.y) < arrivalThreshold;

    if (reachedTargetAxis && alignedX && alignedY) { // 如果在该轴向到达了目标，并且两个轴都对齐了
        m_position = m_pixelTargetForTileMove; // 精确对齐到格子中心
        m_sprite.setPosition(m_position);
        m_isMovingToNextTile = false;
        // std::cout << "AI reached center of target tile (Strict Axis Movement)." << std::endl;
    } else if ( Tank::get_position() == currentPos && !reachedTargetAxis) { // 如果Tank::move没有移动坦克 (比如撞墙了)
        // 且我们还没有在逻辑上到达目标轴的指定点，那么就停止这次移动尝试，让AI重新决策
        m_isMovingToNextTile = false;
        // std::cout << "AI movement blocked by collision before reaching target axis point." << std::endl;
    }
}
void AITank::setStrategicTargetTile(sf::Vector2i targetTile) {
    m_strategicTargetTileCoordinate = targetTile;
    m_hasStrategicTarget = (targetTile.x != -1 && targetTile.y != -1); // 假设 (-1,-1) 是无效或未设置
    if (m_hasStrategicTarget) {
        std::cout << "AI Tank: Strategic target tile set to (" << targetTile.x << ", " << targetTile.y << ")" << std::endl;
        // 当战略目标更新时，可能需要重置当前移动状态，以便重新决策
        m_isMovingToNextTile = false;
    } else {
        std::cout << "AI Tank: Strategic target cleared or invalid." << std::endl;
    }
}

void AITank::decideNextAction(const Map& map, const Tank* playerTankRef) {
    if (m_isMovingToNextTile) {
        return; // 正在移动中，不进行新的移动决策
    }

    if (!m_hasStrategicTarget) {
        //std::cout << "AI Tank: No strategic target. Idling." << std::endl;
        // 可以选择巡逻或待机
        return;
    }

    // --- 目标是 m_strategicTargetTileCoordinate ---
    sf::Vector2i currentTile = getCurrentTile(map);

    // 如果已经在目标瓦片，则不需要移动决策 (可以考虑攻击逻辑)
    if (currentTile.x == m_strategicTargetTileCoordinate.x && currentTile.y == m_strategicTargetTileCoordinate.y) {
        std::cout << "AI Tank: Already at strategic target tile. Considering attack or idle." << std::endl;
        // TODO: 在这里加入攻击逻辑，如果目标是大本营且可以攻击
        // 例如，调整方向对准大本营（如果大本营不是一个点而是一个区域，需要更复杂逻辑）
        // 然后尝试射击
        return;
    }

    // --- 寻路逻辑 (目前是占位符，需要用A*等算法替换) ---
    // 目的是找到从 currentTile 到 m_strategicTargetTileCoordinate 的路径上的第一个移动方向
    Direction chosenDir = m_direction; // 默认保持当前方向
    bool foundValidMove = false;

    // 极简的寻路：尝试直接朝目标移动一格
    // 优先级：先匹配Y轴，再匹配X轴 (或者反过来，或者基于A*的第一个步骤)
    if (m_strategicTargetTileCoordinate.y < currentTile.y && map.isTileWalkable(currentTile.x, currentTile.y - 1)) {
        chosenDir = Direction::UP;
        foundValidMove = true;
    } else if (m_strategicTargetTileCoordinate.y > currentTile.y && map.isTileWalkable(currentTile.x, currentTile.y + 1)) {
        chosenDir = Direction::DOWN;
        foundValidMove = true;
    } else if (m_strategicTargetTileCoordinate.x < currentTile.x && map.isTileWalkable(currentTile.x - 1, currentTile.y)) {
        chosenDir = Direction::LEFT;
        foundValidMove = true;
    } else if (m_strategicTargetTileCoordinate.x > currentTile.x && map.isTileWalkable(currentTile.x + 1, currentTile.y)) {
        chosenDir = Direction::RIGHT;
        foundValidMove = true;
    }

    if (!foundValidMove) {
        // 如果不能直接朝目标移动，尝试选择任意一个可行的方向 (非常基础的“解除卡死”逻辑)
        // 真正的寻路会给出更好的选择
        std::vector<Direction> possibleMoves;
        if (map.isTileWalkable(currentTile.x, currentTile.y - 1)) possibleMoves.push_back(Direction::UP);
        if (map.isTileWalkable(currentTile.x, currentTile.y + 1)) possibleMoves.push_back(Direction::DOWN);
        if (map.isTileWalkable(currentTile.x - 1, currentTile.y)) possibleMoves.push_back(Direction::LEFT);
        if (map.isTileWalkable(currentTile.x + 1, currentTile.y)) possibleMoves.push_back(Direction::RIGHT);

        if (!possibleMoves.empty()) {
            // 简单地选择与当前方向不同的第一个可行方向，或者随机选
            for(Direction dir : possibleMoves) {
                if(dir != m_direction) { // 避免原地踏步或立即回头 (除非是唯一选择)
                    chosenDir = dir;
                    foundValidMove = true;
                    break;
                }
            }
            if(!foundValidMove) { // 如果所有可行方向都与当前方向相同（比如只有一个出口）
                chosenDir = possibleMoves[0];
                foundValidMove = true;
            }
        }
    }

    if (foundValidMove) {
        sf::Vector2i nextTileToMoveTo = currentTile;
        switch (chosenDir) {
            case Direction::UP:    nextTileToMoveTo.y--; break;
            case Direction::DOWN:  nextTileToMoveTo.y++; break;
            case Direction::LEFT:  nextTileToMoveTo.x--; break;
            case Direction::RIGHT: nextTileToMoveTo.x++; break;
        }

        setDirection(chosenDir);
        m_intendedDirectionForTileMove = chosenDir;
        m_pixelTargetForTileMove = getPixelCenterForTile(nextTileToMoveTo.x, nextTileToMoveTo.y, map);
        m_isMovingToNextTile = true;

        //std::cout << "AI (Tile Move) decided to move " << static_cast<int>(chosenDir)
        //          << " towards tile (" << nextTileToMoveTo.x << "," << nextTileToMoveTo.y << ")" << std::endl;
    } else {
        //std::cout << "AI (Tile Move): No valid move found towards strategic target or stuck." << std::endl;
    }
    // --- 寻路逻辑占位符结束 ---
}
