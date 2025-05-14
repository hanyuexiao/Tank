//
// Created by admin on 2025/5/12.
//
#include "PlayerTank.h"

PlayerTank::PlayerTank(sf::Vector2f startPosition, Direction startDirection, int frameWidth, int frameHeight)
        : Tank(startPosition, startDirection, frameWidth, frameHeight) {
    // 如果 PlayerTank 有特定的纹理，可以在这里覆盖基类的纹理加载，
    // 或者基类 Tank 的 loadTextures 设计得更灵活一些（比如接收路径前缀）。
    // 例如，如果玩家坦克的纹理文件名是 "player_tank_left_0.png" 等，
    // 你可能需要一个不同的 loadTextures 版本或机制。
    // 当前 Tank::loadTextures 使用的是固定路径 "tank/tank_..."
    // 如果玩家坦克也用这个路径，那就不需要额外操作。
    std::cout << "PlayerTank created." << std::endl;
}
