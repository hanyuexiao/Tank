#ifndef TANKS_GAME_H
#define TANKS_GAME_H

#include <vector>               // 用于 std::vector
#include <string>               // 用于 std::string
#include <map>                  // 用于 std::map
#include <memory>               // 用于 std::unique_ptr
#include <SFML/Graphics.hpp>    // SFML 图形库
#include <SFML/System/Clock.hpp> // SFML 时钟

#include "heads.h"              // 项目通用头文件 (应确保 SFML 和其他基础库已在此包含)
#include "Map.h"                // 地图类定义
#include "tank.h"               // 坦克基类定义
#include "Bullet.h"             // 子弹类定义
#include "common.h"             // 通用定义 (如 Direction 枚举)
#include "nlohmann/json.hpp"    // JSON 库，用于配置文件解析

// 前向声明 (Forward declarations)
class PlayerTank;               // 玩家坦克类
class AITank;                   // AI 坦克类
// class Tools;                 // 如果 Game 类需要直接管理 Tools 基类指针，则取消注释

// 游戏状态枚举
enum class GameState {
    MainMenu,                   // 主菜单状态
    Playing1P,                  // 单人游戏进行中
    Playing2P,                  // 双人游戏进行中 (如果支持)
    Settings,                   // 设置菜单
    GameOver                    // 游戏结束状态
};

class Game {

public:
    // 构造函数与析构函数
    Game();
    ~Game();

    // 游戏流程控制方法
    void init();                // 初始化游戏资源 (加载配置、纹理、地图、创建初始对象等)
    void run();                 // 启动并运行游戏主循环
    void end();                 // 结束游戏，进行清理 (如果需要)

    // 窗口状态
    bool isWindowOpen() const { return window.isOpen(); } // 检查游戏窗口是否仍然打开

    // 游戏对象管理
    void addBullet(std::unique_ptr<Bullet> bullet);     // 将新创建的子弹添加到游戏世界

    // --- Getter 方法，用于游戏逻辑和对象交互 ---
    std::vector<std::unique_ptr<Tank>>& getAllTanksForModification() { return m_all_tanks; } // 获取所有坦克的列表 (可修改)
    PlayerTank* getPlayerTank() const { return m_playerTankPtr; }   // 获取玩家坦克对象的指针
    Map& getMap() { return m_map; }                                 // 获取对地图对象的引用
    const Map& getMap() const { return m_map; }                     // 获取对地图对象的常量引用

    // --- 纹理资源获取方法 ---
    // 从缓存中获取单个纹理 (例如：道具、地图瓦片、单个子弹纹理)
    const sf::Texture& getTexture(const std::string& key) const;
    // 从缓存中获取特定坦克类型和方向的动画帧纹理列表
    const std::vector<sf::Texture>& getTankTextures(const std::string& tankType, Direction dir) const;

private:
    // --- 内部初始化与加载方法 ---
    bool loadConfig(const std::string& configPath);         // 从指定路径加载 JSON 配置文件并解析
    bool loadTextureFromJson(const std::string& key, const std::string& path); // 根据路径加载单个纹理到缓存

    // --- 核心游戏循环方法 ---
    void Handling_events(sf::Time dt); // 处理窗口事件和用户输入 (dt: 帧间隔时间)
    void update(sf::Time dt);          // 更新游戏世界中所有对象的状态和逻辑 (dt: 帧间隔时间)
    void render();                     // 将游戏世界渲染到窗口上

    // --- 碰撞处理 ---
    void resolveTankCollision(Tank* tank1, Tank* tank2);   // 解决坦克之间的碰撞

    // --- 游戏核心数据成员 ---
    sf::RenderWindow window;           // SFML 渲染窗口
    GameState state;                   // 当前的游戏状态
    Map m_map;                         // 游戏地图对象

    // --- 游戏实体管理 ---
    std::vector<std::unique_ptr<Tank>> m_all_tanks;     // 存储游戏中所有坦克 (玩家和AI)
    PlayerTank* m_playerTankPtr;                        // 指向玩家坦克对象的原始指针 (方便快速访问)
    std::vector<std::unique_ptr<Bullet>> m_bullets;     // 存储游戏中所有活动的子弹
    // std::vector<std::unique_ptr<Tools>> m_tools;      // (示例) 存储游戏中所有道具

    // --- 资源缓存 ---
    nlohmann::json m_configJson;                        // 存储从 config.json 解析后的 JSON 数据
    std::map<std::string, sf::Texture> m_textureCache;  // 缓存加载的单个纹理 (键: 纹理标识符, 值: 纹理对象)
    // 用于道具、地图瓦片、特定方向的子弹等

    // 坦克纹理缓存 (键1: 坦克类型字符串, 键2: 方向枚举, 值: 该方向的动画帧纹理列表)
    std::map<std::string, std::map<Direction, std::vector<sf::Texture>>> m_tankTextureCache;

    // --- 游戏统计与状态 ---
    int score;                         // 玩家得分
    int life;                          // 玩家剩余生命/机会

    // --- 计时与同步 ---
    sf::Clock clock;                   // SFML 时钟，用于计算帧间隔时间 (deltaTime)
};

#endif //TANKS_GAME_H
