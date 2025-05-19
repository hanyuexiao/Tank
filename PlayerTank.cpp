// PlayerTank.cpp
// 创建者: admin, 日期: 2025/5/12 (此日期为原始文件中的注释，保留)
// 功能: 实现 PlayerTank 类的成员函数，主要为玩家控制的坦克。

#include "PlayerTank.h" // 包含 PlayerTank 类的头文件定义
#include "Game.h"       // 包含 Game 类的头文件，因为构造函数需要 Game 引用
// (即使基类 Tank 的头文件可能已间接包含，显式包含更清晰)
#include <iostream>     // 用于调试输出 (例如 std::cout)

int PlayerTank::construction_count = 0;
// PlayerTank 构造函数实现
// 参数:
//   startPosition - 玩家坦克的初始世界坐标
//   startDirection - 玩家坦克的初始逻辑方向 (例如 Direction::UP)
//   game - 对 Game 对象的引用，用于基类 Tank 从中获取纹理等资源
//   speed - 玩家坦克的移动速度 (像素/秒)
//   frameWidth - 坦克纹理单帧的宽度 (像素)
//   frameHeight - 坦克纹理单帧的高度 (像素)
//   initialHealth - 玩家坦克的初始生命值
//   initialArmor - 玩家坦克的初始护甲值
PlayerTank::PlayerTank(sf::Vector2f startPosition,
                       Direction startDirection,
                       Game& game, // 接收对 Game 对象的引用
                       float speed,
                       int frameWidth,
                       int frameHeight,
                       int initialHealth,
                       int initialArmor)
// 调用基类 Tank 的构造函数，并传递必要的参数:
// 1. startPosition: 初始位置
// 2. startDirection: 初始方向
// 3. "player": 坦克类型字符串。这个字符串将用于 Game 类从 JSON 配置中
//              查找并加载此玩家坦克对应的纹理。
//              确保 "player" 与 config.json 中定义的键名一致。
// 4. game: 对 Game 对象的引用，基类将用它来获取纹理。
// 5. speed: 移动速度。
// 6. frameWidth: 纹理帧宽度。
// 7. frameHeight: 纹理帧高度。
// 8. initialHealth: 初始生命值。
// 9. initialArmor: 初始护甲值。
        : Tank(startPosition,
               startDirection,
               "player",            // 坦克类型固定为 "player"
               game,
               speed,
               frameWidth,
               frameHeight,
               initialHealth,
               initialArmor,
               0)
{
    construction_count++;
    // 构造函数体

    // 调试输出，确认 PlayerTank 对象已创建，并显示其类型 (通过基类方法获取)
    // std::cout << "PlayerTank (type '" << getTankType() << "') created at ("
    //           << startPosition.x << ", " << startPosition.y << ")." << std::endl;

    // 如果 PlayerTank 有特定于玩家的初始化逻辑（除了基类 Tank 已完成的之外），
    // 例如，玩家坦克可能有不同于默认坦克的射击冷却时间、攻击力或特殊能力，
    // 可以在这里进行设置。
    // 示例：
    // this->m_baseShootCooldown = sf::seconds(0.35f); // 玩家射速可能比AI默认更快
    // this->m_shootCooldown = this->m_baseShootCooldown;
    // this->m_baseAttackPower = 30; // 玩家攻击力可能比AI默认更高
    // this->m_currentAttackPower = this->m_baseAttackPower;
}

// 如果 PlayerTank 类需要覆盖基类 Tank 中的 update, setDirection, 或其他虚函数，
// 并且这些覆盖版本有特定于 PlayerTank 的行为（例如，处理玩家特有的动画状态或输入），
// 那么这些方法的实现会放在这里。
//
// 例如，如果 PlayerTank 有一个特殊的 "冲刺" 状态，可能会改变其动画或行为：
//
// void PlayerTank::update(sf::Time dt, Game& game) {
//     // 首先调用基类的 update 方法处理通用逻辑 (如动画、buff计时器)
//     Tank::update(dt, game);
//
//     // 然后添加 PlayerTank 特有的更新逻辑
//     // if (m_isDashing) {
//     //     // 处理冲刺状态的逻辑...
//     //     // 可能需要根据冲刺状态从 game 对象获取不同的纹理或应用效果
//     // }
//     // 处理玩家输入以激活特殊技能等...
// }
//
// void PlayerTank::setDirection(Direction dir, Game& game) {
//     // 首先调用基类的 setDirection 方法处理通用逻辑 (如更新方向和基础纹理)
//     Tank::setDirection(dir, game);
//
//     // 如果 PlayerTank 在不同方向有完全不同的外观（不仅仅是基类处理的动画帧），
//     // 可以在这里添加额外的逻辑。但通常，基类的 setDirection 配合 Game 的纹理缓存已足够。
// }

// 注意：如果 PlayerTank 在 update 和 setDirection 等方面的行为与基类 Tank 完全一致，
// (即，它只是使用由 "player" 类型决定的纹理，而没有其他特殊视觉或行为逻辑)，
// 那么就不需要在 PlayerTank.h 中声明这些方法的覆盖版本，也不需要在这里提供它们的实现。
// C++ 的继承机制会自动调用基类 Tank 中已经修改过的、接收 Game& game 参数的版本。
