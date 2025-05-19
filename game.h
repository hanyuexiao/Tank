#ifndef TANKS_GAME_H
#define TANKS_GAME_H

// 统一包含所有必要的头文件
#include "heads.h"        // 项目通用头文件 (SFML, iostream, json, etc.)
#include "Map.h"          // 地图类
#include "tank.h"         // 坦克基类
#include "Bullet.h"       // 子弹类
#include "common.h"       // 通用定义 (如 Direction 枚举)
#include "AITank.h"       // AI坦克类
#include "Tools.h"        // 道具基类
#include <random>         // For std::mt19937

// 前向声明 (Forward declarations)
class PlayerTank; // 玩家坦克类

// 游戏状态枚举
enum class GameState {
    MainMenu,
    Playing1P,
    LevelTransition, // 新增：用于显示关卡切换信息的状态
    GameOver
    // Settings, Playing2P 等可以保留用于未来扩展
};

// AI坦克类型配置结构体 (保持不变)
struct AITankTypeConfig {
    std::string typeName;
    std::string textureKey;
    float baseSpeed;
    int baseHealth;
    int baseAttack;
    int frameWidth;
    int frameHeight;
    int scoreValue;
};

class Game {
public:
    // =========================================================================
    // 构造函数与析构函数
    // =========================================================================
    Game();
    ~Game();

    // =========================================================================
    // 游戏流程控制方法
    // =========================================================================
    void init();
    void run();
    void end();

    // =========================================================================
    // Getter 方法 - 游戏状态与对象访问
    // =========================================================================
    bool isWindowOpen() const { return window.isOpen(); }
    Map& getMap() { return m_map; }
    const Map& getMap() const { return m_map; }
    PlayerTank* getPlayerTank() const { return m_playerTankPtr; }
    std::vector<std::unique_ptr<Tank>>& getAllTanksForModification() { return m_all_tanks; }
    Bullet* getAvailableBullet();
    int getCurrentLevel() const { return m_currentLevel; } // 获取当前关卡

    // =========================================================================
    // Getter 方法 - 资源访问
    // =========================================================================
    const sf::Texture& getTexture(const std::string& key) const;
    const std::vector<sf::Texture>& getTankTextures(const std::string& tankType, Direction dir) const;

private:
    // =========================================================================
    // 内部核心逻辑方法
    // =========================================================================
    void Handling_events(sf::Time dt);
    void update(sf::Time dt);
    void render();

    // =========================================================================
    // 内部初始化与加载方法
    // =========================================================================
    bool loadConfig(const std::string& configPath);
    bool loadTextureFromJson(const std::string& key, const std::string& path);
    void loadToolTypesFromConfig();
    void loadAITankConfigs();
    void initializeBulletPool();
    void setupLevel(); // 修改：用于设置或重置当前关卡
    void advanceToNextLevel(); // 新增：进入下一关的逻辑

    // =========================================================================
    // 碰撞处理方法 (保持不变)
    // =========================================================================
    void resolveTankCollision(Tank* tank1, Tank* tank2);
    void resolveTankToolCollision(Tank* tank, Tools* tool);

    // =========================================================================
    // 游戏对象生成与管理方法 (保持不变)
    // =========================================================================
    void spawnRandomTool();
    void updateTools(sf::Time dt);
    void spawnNewAITank();
    void updateAITankSpawning(sf::Time dt);
    GameState getCurrentState() const { return state; }

    // =========================================================================
    // 核心游戏数据成员
    // =========================================================================
    sf::RenderWindow window;
    GameState state;
    Map m_map;
    sf::Clock clock;
    std::mt19937 m_rng; // 随机数生成器，用于地图生成等

    // =========================================================================
    // 游戏实体管理 (保持不变)
    // =========================================================================
    std::vector<std::unique_ptr<Tank>> m_all_tanks;
    PlayerTank* m_playerTankPtr;
    std::vector<std::unique_ptr<Bullet>> m_bulletPool;
    std::vector<std::unique_ptr<Tools>> m_tools;

    // =========================================================================
    // 资源缓存与配置 (保持不变)
    // =========================================================================
    nlohmann::json m_configJson;
    std::map<std::string, sf::Texture> m_textureCache;
    std::map<std::string, std::map<Direction, std::vector<sf::Texture>>> m_tankTextureCache;
    sf::Font m_uiFont;

    // =========================================================================
    // UI 文本元素
    // =========================================================================
    sf::Text m_baseHealthText;
    sf::Text m_scoreText;
    sf::Text m_playerStatsTitleText;
    sf::Text m_playerHealthText;
    sf::Text m_playerArmorText;
    sf::Text m_playerAttackText;
    sf::Text m_playerSpeedText;
    sf::Text m_playerCooldownText;
    sf::Text m_playerDestroyedText;
    sf::Text m_currentLevelText;      // 新增：显示当前关卡
    sf::Text m_levelTransitionMessageText; // 新增：显示 "Level X" 或 "Prepare for next level"
    sf::Time m_levelTransitionDisplayTimer; // 新增：控制关卡切换信息显示时间

    // =========================================================================
    // 游戏统计与状态
    // =========================================================================
    int score;
    // int life; // 保留，如果需要生命机制
    int m_currentLevel; // 新增：当前关卡号，从1开始

    // =========================================================================
    // 道具生成相关配置与状态 (保持不变)
    // =========================================================================
    sf::Time m_toolSpawnInterval;
    sf::Time m_toolSpawnTimer;
    std::vector<std::string> m_availableToolTypes;

    // =========================================================================
    // AI坦克生成相关配置与状态 (保持不变)
    // =========================================================================
    sf::Time m_aiTankSpawnInterval;
    sf::Time m_aiTankSpawnTimer;
    int m_maxActiveAITanks;
    std::map<std::string, AITankTypeConfig> m_aiTypeConfigs;
    std::vector<std::string> m_availableAITankTypeNames;
    float m_defaultAITankSpeed;
    int m_defaultAIBaseHealth;
    int m_defaultAIBaseAttack;
    int m_defaultAIFrameWidth;
    int m_defaultAIFrameHeight;
    int m_defaultAIScoreValue;

    // =========================================================================
    // 常量
    // =========================================================================
    const size_t INITIAL_BULLET_POOL_SIZE = 100;
    // 新增：关卡分数阈值
    static const int SCORE_THRESHOLD_LEVEL_2 = 200;
    static const int SCORE_THRESHOLD_LEVEL_3 = 500; // 总分达到5000进入第三关 (2000 for L2 + 3000 more)
    // 或者你可以设定为每关独立的增量分数
    static const int MAX_LEVEL = 3; // 最大关卡数
};

#endif //TANKS_GAME_H
