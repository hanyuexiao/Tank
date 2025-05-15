// Created by admin on 2025/5/12.
//

#ifndef AITANK_H
#define AITANK_H

#include "Tank.h"
#include "Map.h"  // For Map reference

class Game;  // 保留一个前向声明，删除重复行

class AITank : public Tank {
public:
    AITank(sf::Vector2f startPosition, Direction direction, float speed = 80.f, int frameWidth = 50, int frameHeight = 50,int inihp = 100);

    // AI决策：决定下一步要朝哪个方向移动（或者是否射击等）
    // 这个函数会被 Game::update 定期调用
    void decideNextAction(const Map& map, const Tank* playerTankRef /* 或其他目标信息 */); // 补全注释闭合
    void setStrategicTargetTile(sf::Vector2i targetTile);
    // 每帧调用，处理正在进行的格子间移动
    void updateMovementBetweenTiles(sf::Time dt, const Map& map);

    bool isMoving() const { return m_isMovingToNextTile; }

private:
    bool m_isMovingToNextTile;         // 坦克当前是否正在从一个格子移动到下一个格子
    sf::Vector2f m_pixelTargetForTileMove; // 当前格子间移动的目标像素位置 (下一个格子的中心)
    Direction m_intendedDirectionForTileMove; // 本次格子移动的预定方向
    sf::Vector2i m_strategicTargetTileCoordinate;
    bool m_hasStrategicTarget{};
    // 辅助函数
    static sf::Vector2f getPixelCenterForTile(int tileX, int tileY, const Map& map) ;
    sf::Vector2i getCurrentTile(const Map& map) const;
};

#endif //AITANK_H