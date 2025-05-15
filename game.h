#ifndef TANKS_GAME_H
#define TANKS_GAME_H

#include <optional>       // C++17 feature, though not explicitly used in current public interface, good to keep if planned
#include "heads.h"        // 包含项目通用的头文件 (SFML, iostream, vector, map, memory, etc.)
#include "Map.h"          // 包含地图类定义
#include "tank.h"         // 包含坦克基类定义
#include "Bullet.h"       // 包含子弹类定义
#include "common.h"       // 包含 Direction 等通用枚举 (虽然 tank.h 和 bullet.h 可能也包含了)

// 前向声明，减少编译依赖，提高编译速度
class PlayerTank;     // 玩家坦克类，继承自 Tank
class AITank;         // AI 坦克类，继承自 Tank
// class Tools;       // 如果 Game 类需要管理 Tools 基类指针，可以前向声明 Tools
// (或者在 Game.cpp 中包含 Tools.h 和其子类头文件)

// 游戏状态枚举，用于控制游戏的不同阶段或模式
enum class GameState {
    MainMenu,         // 主菜单状态
    Playing1P,        // 单人游戏进行中状态
    Playing2P,        // 双人游戏进行中状态 (如果支持)
    Settings,         // 设置菜单状态
    GameOver          // 游戏结束状态
};

class Game {

public:
    // 构造函数与析构函数
    Game();           // 构造函数，初始化游戏窗口、状态等
    ~Game();          // 析构函数，用于清理资源 (虽然很多资源由智能指针管理)

    // 游戏流程控制方法
    void init();      // 初始化游戏资源 (加载纹理、地图、创建初始对象等)
    void run();       // 启动并运行游戏主循环
    void end();       // 结束游戏，进行清理和状态保存 (如果需要)

    // 资源加载与获取
    void load_date(); // 加载游戏数据/资源，例如纹理、声音等 (函数名建议用 loadData 或 loadResources)
    bool isWindowOpen() const { return window.isOpen(); } // 检查游戏窗口是否仍然打开
    const sf::Texture& getBulletTexture(Direction dir) const; // 获取指定方向的子弹纹理

    // 游戏对象管理
    void addBullet(std::unique_ptr<Bullet> bullet); // 将新创建的子弹添加到游戏世界中

    // --- 为道具和碰撞逻辑添加的 Getter ---
    // 获取所有坦克的列表 (用于道具效果如手雷，或碰撞检测)
    // 返回引用，允许外部修改容器 (例如手雷移除坦克)
    std::vector<std::unique_ptr<Tank>>& getAllTanksForModification() { return m_all_tanks; }
    // 获取玩家坦克对象的指针 (用于AI目标、道具效果判断等)
    PlayerTank* getPlayerTank() const { return m_playerTankPtr; }
    // 获取对地图对象的引用 (例如AI寻路、子弹与地图碰撞)
    Map& getMap() { return m_map; }
    const Map& getMap() const { return m_map; }


private:
    // 内部核心方法
    void Handling_events(sf::Time dt); // 处理窗口事件和用户输入，dt 是帧间隔时间 (函数名建议用 handleEvents 或 processInput)
    void update(sf::Time dt);          // 更新游戏世界中所有对象的状态和逻辑，dt 是帧间隔时间
    void render();                     // 将游戏世界渲染到窗口上

    void resolveTankCollision(Tank* tank1, Tank* tank2); // 解决坦克之间的碰撞

    // 游戏核心数据
    sf::RenderWindow window;           // SFML 渲染窗口，游戏的主要画布
    GameState state;                   // 当前的游戏状态 (MainMenu, Playing, GameOver, etc.)
    Map m_map;                         // 游戏地图对象

    // 游戏实体管理
    std::vector<std::unique_ptr<Tank>> m_all_tanks; // 存储游戏中所有坦克 (玩家和AI) 的智能指针列表
    PlayerTank* m_playerTankPtr;       // 指向玩家坦克对象的原始指针 (方便快速访问，生命周期由 m_all_tanks 中的 unique_ptr 管理)
    std::vector<std::unique_ptr<Bullet>> m_bullets; // 存储游戏中所有活动子弹的智能指针列表
    // std::vector<std::unique_ptr<Tools>> m_tools; // 示例：用于存储游戏中所有道具的智能指针列表

    // 资源缓存
    std::map <Direction, sf::Texture> m_bullet_textures; // 缓存不同方向的子弹纹理，以 Direction 枚举为键

    // 游戏统计与状态 (示例，你可以根据需要添加更多)
    int score;                         // 玩家得分
    int life;                          // 玩家剩余生命/机会 (如果玩家坦克被摧毁后可以复活)

    // 计时与同步
    sf::Clock clock;                   // SFML 时钟，用于计算帧间隔时间 (deltaTime)
};

#endif //TANKS_GAME_H