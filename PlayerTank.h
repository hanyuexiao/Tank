// PlayerTank.h
// Created by admin on 2025/5/12.
//

#ifndef TANKS_PLAYERTANK_H
#define TANKS_PLAYERTANK_H

#include "tank.h"   // 包含基类 Tank 的定义
#include <string>   // 包含 std::string (虽然 tankType 可能硬编码)

// 前向声明 Game 类，因为 Tank 基类的构造函数等现在需要 Game 引用
class Game;

class PlayerTank : public Tank {
public:
    // MODIFIED: 构造函数签名已更改
    // 参数:
    //   startPosition - 玩家坦克的初始位置
    //   startDirection - 玩家坦克的初始方向
    //   game - 对 Game 对象的引用，用于获取纹理和其他游戏上下文
    //   frameWidth - (可选) 坦克纹理单帧宽度，如果与基类默认值不同
    //   frameHeight - (可选) 坦克纹理单帧高度，如果与基类默认值不同
    //   initialHealth - (可选) 初始生命值
    //   initialArmor - (可选) 初始护甲值
    PlayerTank(sf::Vector2f startPosition,
               Direction startDirection,
               Game& game, // 新增：Game 对象的引用
               float speed = 120.f,           // 玩家坦克可以有自己的默认速度
               int frameWidth = 50,         // 默认帧宽
               int frameHeight = 50,        // 默认帧高
               int initialHealth = 100,     // 默认初始生命值
               int initialArmor = 1);       // 玩家坦克初始可以有1点护甲

    // PlayerTank 特有的方法可以放在这里，例如:
    // void activateShield();
    // void specialAbility();

    // 如果 PlayerTank 需要覆盖 update 或 setDirection 并且有特定于 PlayerTank 的纹理逻辑，
    // 那么这些方法的签名也需要与基类 Tank 中修改后的一致 (即包含 Game& game 参数)。
    // 例如:
    // void update(sf::Time dt, Game& game) override;
    // void setDirection(Direction dir, Game& game) override;
    // 但如果 PlayerTank 的行为与 Tank 在这些方面一致，则不需要覆盖。
};

#endif //TANKS_PLAYERTANK_H
