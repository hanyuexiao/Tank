#ifndef TANKS_GAME_H
#define TANKS_GAME_H

#include "heads.h"
#include "Map.h"
#include "tank.h"
#include "Bullet.h"
#include "common.h"
#include "nlohmann/json.hpp"

// 包含所有具体的道具类头文件
#include "Tools.h"
#include "AddArmor.h"
#include "AddAttack.h"
#include "AddAttackSpeed.h"
#include "AddSpeed.h"
#include "Grenade.h"
#include "SlowDownAI.h"
// 包含AI坦克头文件
#include "AITank.h"

struct AITankTypeConfig {
    std::string typeName; // 例如 "ai_default", "ai_fast"
    std::string textureKey;
    float baseSpeed;
    int baseHealth;
    int baseAttack;
    int frameWidth;
    int frameHeight;
    int scoreValue;
};
// 前向声明 (Forward declarations)
class PlayerTank;
// class AITank; // AITank.h 已经被包含了

// 游戏状态枚举
enum class GameState {
    MainMenu,
    Playing1P,
    Playing2P,
    Settings,
    GameOver
};

class Game {

public:
    // 构造函数与析构函数
    Game();
    ~Game();

    // 游戏流程控制方法
    void init();
    void run();
    void end();

    // 窗口状态
    bool isWindowOpen() const { return window.isOpen(); }

    // 游戏对象管理
    void addBullet(std::unique_ptr<Bullet> bullet);

    // --- Getter 方法，用于游戏逻辑和对象交互 ---
    std::vector<std::unique_ptr<Tank>>& getAllTanksForModification() { return m_all_tanks; }
    PlayerTank* getPlayerTank() const { return m_playerTankPtr; }
    Map& getMap() { return m_map; }
    const Map& getMap() const { return m_map; }
    Bullet* getAvailableBullet();
    // --- 纹理资源获取方法 ---
    const sf::Texture& getTexture(const std::string& key) const;
    const std::vector<sf::Texture>& getTankTextures(const std::string& tankType, Direction dir) const;

private:
    // --- 内部初始化与加载方法 ---
    bool loadConfig(const std::string& configPath);
    bool loadTextureFromJson(const std::string& key, const std::string& path);
    void loadToolTypesFromConfig(); // 从配置加载可用道具类型

    // --- 核心游戏循环方法 ---
    void Handling_events(sf::Time dt);
    void update(sf::Time dt);
    void render();

    // --- 碰撞处理 ---
    void resolveTankCollision(Tank* tank1, Tank* tank2);
    void resolveTankToolCollision(Tank* tank, Tools* tool); // 处理坦克与道具碰撞

    // --- 游戏核心数据成员 ---
    sf::RenderWindow window;
    GameState state;
    Map m_map;

    // --- 游戏实体管理 ---
    std::vector<std::unique_ptr<Tank>> m_all_tanks;
    PlayerTank* m_playerTankPtr;
    std::vector<std::unique_ptr<Bullet>> m_bullets;
    std::vector<std::unique_ptr<Tools>> m_tools; // 存储游戏中所有道具
    std::vector<std::unique_ptr<Bullet>> m_bulletPool; // 存储游戏中所有砖墙
    const size_t INITIAL_BULLET_POOL_SIZE = 100;
    void initializeBulletPool();
    // --- 资源缓存 ---
    nlohmann::json m_configJson;
    std::map<std::string, sf::Texture> m_textureCache;
    std::map<std::string, std::map<Direction, std::vector<sf::Texture>>> m_tankTextureCache;

    // --- 游戏统计与状态 ---
    int score;
    int life;

    // --- 计时与同步 ---
    sf::Clock clock;

    // --- 道具生成相关 ---
    sf::Time m_toolSpawnInterval;       // 道具生成的时间间隔
    sf::Time m_toolSpawnTimer;          // 道具生成计时器
    std::vector<std::string> m_availableToolTypes; // 可生成的道具类型键名列表

    void spawnRandomTool();             // 生成随机道具的方法
    void updateTools(sf::Time dt);      // 更新道具状态和碰撞的方法

    // --- AI坦克生成相关 ---
    sf::Time m_aiTankSpawnInterval;     // AI坦克生成的时间间隔
    sf::Time m_aiTankSpawnTimer;        // AI坦克生成计时器
    int m_maxActiveAITanks;             // 屏幕上最大AI坦克数量

    std::string m_defaultAITankType;    // 默认生成的AI坦克类型 (从config加载)
    float m_defaultAITankSpeed;         // 默认生成的AI坦克速度
    int m_defaultAIBaseHealth; // Game 成员变量
    int m_defaultAIBaseAttack;
    void spawnNewAITank();              // 生成新AI坦克的方法
    void updateAITankSpawning(sf::Time dt); // 更新AI坦克生成计时器
    int m_defaultAIFrameWidth;
    int m_defaultAIFrameHeight;
    int m_defaultAIScoreValue;

    std::map<std::string ,AITankTypeConfig> m_aiTypeConfigs; // AI坦克类型配置表
    std::vector<std::string >m_availableAITankTypeNames; // 可生成的AI坦克类型键名列表

    void loadAITankConfigs(); // 加载AI坦克类型配置的方法



};

#endif //TANKS_GAME_H