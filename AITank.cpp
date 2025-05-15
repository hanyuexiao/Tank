//
// Created by admin on 2025/5/12.
//

#include "AITank.h"
#include "Bullet.h" // 如果 decideAction 直接创建子弹

AITank::AITank(sf::Vector2f startPosition, Direction direction, float speed, int frameWidth, int frameHeight,int inihp)
        : Tank(startPosition, direction, speed, frameWidth, frameHeight,inihp),
          m_isMovingToNextTile(false) {
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

    // 确保视觉方向与意图方向一致 (如果AI在格子间移动时方向不会被其他逻辑改变，这可能不是必须的)
    if (get_Direction() != m_intendedDirectionForTileMove) {
        setDirection(m_intendedDirectionForTileMove);
    }

    sf::Vector2f currentPos = get_position();
    sf::Vector2f directionToTargetPixel = m_pixelTargetForTileMove - currentPos;
    float distanceToTargetPixel = std::sqrt(directionToTargetPixel.x * directionToTargetPixel.x + directionToTargetPixel.y * directionToTargetPixel.y);

    float arrivalThreshold = 1.5f; // 像素阈值

    if (distanceToTargetPixel <= arrivalThreshold) {
        m_position = m_pixelTargetForTileMove; // 精确对齐到格子中心
        m_sprite.setPosition(m_position);
        m_isMovingToNextTile = false;
        //std::cout << "AI reached center of target tile." << std::endl;
        return;
    }

    sf::Vector2f normalizedDir(0.f, 0.f);
    if (distanceToTargetPixel > 0) {
        normalizedDir.x = directionToTargetPixel.x / distanceToTargetPixel;
        normalizedDir.y = directionToTargetPixel.y / distanceToTargetPixel;
    }

    sf::Vector2f movementThisFrame = normalizedDir * m_speed * dt.asSeconds();
    sf::Vector2f desiredNextPixelPos;

    // 防止过冲：如果本帧的移动量大于到目标的距离，则直接移动到目标
    if (movementThisFrame.x * movementThisFrame.x + movementThisFrame.y * movementThisFrame.y >= distanceToTargetPixel * distanceToTargetPixel) {
        desiredNextPixelPos = m_pixelTargetForTileMove;
    } else {
        desiredNextPixelPos = currentPos + movementThisFrame;
    }

    // *** 使用基类的 Tank::move 进行实际移动和碰撞检测 ***
    sf::Vector2f originalPosBeforeMove = currentPos;
    Tank::move(desiredNextPixelPos, map); // Tank::move 会更新 m_position (如果成功)
    sf::Vector2f actualNewPos = get_position();

    // 检查移动是否成功，以及是否因为碰撞而没有移动到预期的 desiredNextPixelPos
    if (actualNewPos == originalPosBeforeMove && distanceToTargetPixel > arrivalThreshold) {
        // 坦克尝试移动，但 Tank::move 因为碰撞阻止了它。
        // 这意味着 desiredNextPixelPos (即使它仍在当前格子内或者非常接近)
        // 会导致坦克的包围盒与不可通行的瓦片碰撞。
        // 这通常发生在坦克试图“挤”过一个狭窄的边缘，或者其包围盒比预期的要大。
        //std::cout << "AI (Tile Move): Tank::move blocked movement towards desired pixel." << std::endl;
        m_isMovingToNextTile = false; // 停止当前移动尝试，让 decideNextAction 重新评估
        // 在更复杂的AI中，这里可能会标记当前方向/路径受阻
    } else if (actualNewPos != desiredNextPixelPos && desiredNextPixelPos == m_pixelTargetForTileMove) {
        // 如果目标是格子中心，但 Tank::move 之后没有精确到达（可能被阻挡了一点点）
        // 并且我们还没到 arrivalThreshold，我们可能仍认为在移动。
        // 但如果已经接近 arrivalThreshold，上面的第一个 if 会处理。
    }


    // 如果移动成功（或者部分成功）后，再次检查是否到达最终的格子中心目标
    // （因为 Tank::move 可能不会精确地移动到 desiredNextPixelPos，如果它导致过冲格子中心的话）
    currentPos = get_position(); // 更新当前位置以进行下一次到达检查
    directionToTargetPixel = m_pixelTargetForTileMove - currentPos;
    distanceToTargetPixel = std::sqrt(directionToTargetPixel.x * directionToTargetPixel.x + directionToTargetPixel.y * directionToTargetPixel.y);

    if (distanceToTargetPixel <= arrivalThreshold) {
        m_position = m_pixelTargetForTileMove; // 再次精确对齐
        m_sprite.setPosition(m_position);
        m_isMovingToNextTile = false;
        //std::cout << "AI reached center of target tile after Tank::move." << std::endl;
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
