// game.cpp

// =========================================================================
// 必要的头文件包含
// =========================================================================
#include "game.h"
#include "PlayerTank.h" // 玩家坦克类
#include "AITank.h"     // AI坦克类 (已在game.h中包含，但显式包含无害)
#include "Bullet.h"     // 子弹类 (已在game.h中包含)
#include "Map.h"        // 地图类 (已在game.h中包含)

// 道具类头文件 (如果Game类中具体创建或引用了它们)
#include "AddArmor.h"
#include "AddAttack.h"
#include "AddAttackSpeed.h"
#include "AddSpeed.h"
#include "Grenade.h"
#include "SlowDownAI.h"

#include <iostream>     // 用于标准输入输出 (例如 std::cout, std::cerr)
#include <fstream>      // 用于文件流操作 (例如 std::ifstream)
#include <vector>       // 用于 std::vector
#include <algorithm>    // 用于 std::remove_if, std::sort 等算法
#include <random>       // 用于随机数生成 (例如 std::random_device, std::mt19937)
#include <string>       // 用于 std::string 和 std::to_string
#include <sstream>      // 用于 std::ostringstream (格式化字符串)
#include <iomanip>      // 用于 std::fixed, std::setprecision (格式化输出)

// =========================================================================
// 构造函数与析构函数
// =========================================================================
Game::Game(): window(sf::VideoMode(1500, 750), "Tank Battle!"),
              state(GameState::MainMenu), // 初始状态可以是MainMenu或直接Playing1P
              score(0),
        // life(3),
              m_map(),
              m_playerTankPtr(nullptr),
              m_rng(std::random_device{}()), // 初始化随机数生成器
              m_currentLevel(1), // 从第一关开始
              m_levelTransitionDisplayTimer(sf::Time::Zero),
              m_toolSpawnInterval(sf::seconds(10.0f)),
              m_toolSpawnTimer(sf::Time::Zero),
              m_aiTankSpawnInterval(sf::seconds(8.0f)), // AI生成间隔可以随关卡调整
              m_aiTankSpawnTimer(sf::Time::Zero),
              m_maxActiveAITanks(5), // 初始AI数量可以少一些
              m_defaultAITankSpeed(30.f),
              m_defaultAIBaseHealth(80),
              m_defaultAIBaseAttack(15),
              m_defaultAIFrameWidth(50),
              m_defaultAIFrameHeight(50),
              m_defaultAIScoreValue(100)
{
    std::cout << "Game constructor called." << std::endl;
}

Game::~Game() {
    std::cout << "Game destructor called." << std::endl;
}

// =========================================================================
// 游戏流程控制方法
// =========================================================================
void Game::init() {
    std::cout << "Game::init() called." << std::endl;
    window.setFramerateLimit(60);       // 设置帧率上限
    window.setVerticalSyncEnabled(true); // 开启垂直同步

    // 加载UI字体
    if (!m_uiFont.loadFromFile("assets/arial.ttf")) { // *** 请确保字体文件路径正确 ***
        std::cerr << "CRITICAL ERROR: Failed to load UI font! Please check path 'assets/arial.ttf'" << std::endl;
        window.close();
        return;
    }
    std::cout << "UI Font loaded successfully." << std::endl;

    // 初始化UI Text对象的基本属性
    float uiPanelX = 1200.f; // 游戏区域宽度1200，UI面板从1200px开始
    float initialY = 30.f;
    float lineSpacing = 28.f;
    unsigned int charSize = 20;
    unsigned int titleCharSize = 22;

    m_baseHealthText.setFont(m_uiFont);
    m_baseHealthText.setCharacterSize(charSize);
    m_baseHealthText.setFillColor(sf::Color::White);

    m_scoreText.setFont(m_uiFont);
    m_scoreText.setCharacterSize(charSize);
    m_scoreText.setFillColor(sf::Color::White);

    m_playerStatsTitleText.setFont(m_uiFont);
    m_playerStatsTitleText.setCharacterSize(titleCharSize);
    m_playerStatsTitleText.setFillColor(sf::Color::Yellow);
    m_playerStatsTitleText.setStyle(sf::Text::Bold);


    m_playerHealthText.setFont(m_uiFont);
    m_playerHealthText.setCharacterSize(charSize);
    m_playerHealthText.setFillColor(sf::Color::White);

    m_playerArmorText.setFont(m_uiFont);
    m_playerArmorText.setCharacterSize(charSize);
    m_playerArmorText.setFillColor(sf::Color::White);

    m_playerAttackText.setFont(m_uiFont);
    m_playerAttackText.setCharacterSize(charSize);
    m_playerAttackText.setFillColor(sf::Color::White);

    m_playerSpeedText.setFont(m_uiFont);
    m_playerSpeedText.setCharacterSize(charSize);
    m_playerSpeedText.setFillColor(sf::Color::White);

    m_playerCooldownText.setFont(m_uiFont);
    m_playerCooldownText.setCharacterSize(charSize);
    m_playerCooldownText.setFillColor(sf::Color::White);

    m_playerDestroyedText.setFont(m_uiFont);
    m_playerDestroyedText.setCharacterSize(charSize);
    m_playerDestroyedText.setFillColor(sf::Color::Red);

    m_currentLevelText.setFont(m_uiFont);
    m_currentLevelText.setCharacterSize(20);
    m_currentLevelText.setFillColor(sf::Color::White);

    m_levelTransitionMessageText.setFont(m_uiFont);
    m_levelTransitionMessageText.setCharacterSize(36);
    m_levelTransitionMessageText.setFillColor(sf::Color::Yellow);
    m_levelTransitionMessageText.setStyle(sf::Text::Bold);

    // 加载配置文件和所有纹理资源
    if (!loadConfig("config.json")) {
        std::cerr << "CRITICAL ERROR: Failed to load game configuration from 'config.json'. Exiting." << std::endl;
        window.close();
        return;
    }
    loadToolTypesFromConfig(); // 从配置中加载可用道具类型
    loadAITankConfigs();       // 从配置中加载AI坦克类型及其属性

    // 从配置中读取AI坦克全局生成参数 (max_active, spawn_interval_seconds)
    if (m_configJson.contains("ai_settings")) {
        m_maxActiveAITanks = m_configJson["ai_settings"].value("max_active", m_maxActiveAITanks);
        float spawnIntervalSeconds = m_configJson["ai_settings"].value("spawn_interval_seconds", m_aiTankSpawnInterval.asSeconds());
        m_aiTankSpawnInterval = sf::seconds(spawnIntervalSeconds);
        std::cout << "Global AI Settings loaded: MaxActive=" << m_maxActiveAITanks
                  << ", SpawnInterval=" << spawnIntervalSeconds << "s" << std::endl;
    } else {
        std::cout << "Warning: Global AI Settings (max_active, etc.) not found in config.json, using hardcoded defaults." << std::endl;
    }

    // 初始化并加载地图
    if(!m_map.loadDimensionsAndTextures(*this)){
        std::cerr << "CRITICAL ERROR: Failed to load map dimensions/textures in Game::init(). Exiting." << std::endl;
        window.close(); return;
    }
    std::cout << "Map dimensions and textures loaded successfully." << std::endl;

    initializeBulletPool();
    setupLevel(); // 设置第一关

    state = GameState::Playing1P; // 游戏初始化完成后进入游玩状态

    // 加载玩家坦克
    // 玩家坦克的初始位置应该在游戏区域内，例如 (100, 窗口高度 - 坦克高度 - 瓦片高度)
    // 假设瓦片大小为50x50, 坦克大小为50x50
    // 基地在底部中间，玩家出生点可以在左下或右下角附近，但要确保在可玩区域内。
    // 窗口高度750，地图瓦片大小通常是50。
    // 基地在最底下一行 m_mapHeight-1 (即第14行, y = 14*50 = 700)
    // 玩家坦克可以放在倒数第二或第三行，偏左或偏右的位置。
    // 例如: (100, 650) 或 (地图宽度*瓦片宽度 - 100, 650)
    // 游戏区域是 0-1200 (X), 0-750 (Y)
    // 玩家出生点 (300, 375) 是地图中间靠左的位置，对于1P可能还行。
    auto player = std::make_unique<PlayerTank>(sf::Vector2f (100.f, 650.f), Direction::UP, *this);
    m_playerTankPtr = player.get();
    m_all_tanks.push_back(std::move(player));
    std::cout << "Player tank (type 'player') created successfully." << std::endl;

    // 初始化子弹对象池
    initializeBulletPool();

    // 重置计时器
    m_toolSpawnTimer = sf::Time::Zero;
    m_aiTankSpawnTimer = sf::Time::Zero;

    // 初始AI坦克 (可以选择在这里创建一些，或者完全依赖定时生成器)
    // spawnNewAITank(); // 示例：游戏开始时生成一个AI

    // 为已存在的AI坦克设置目标 (如果初始就创建了AI)
    sf::Vector2i baseTile = m_map.getBaseTileCoordinate();
    if (baseTile.x != -1 && baseTile.y != -1) {
        for (const auto& tank_ptr : m_all_tanks) {
            if (AITank* ai = dynamic_cast<AITank*>(tank_ptr.get())) {
                ai->setStrategicTargetTile(baseTile);
                std::cout << "Initial AI Tank (type '" << ai->getTankType() << "') target set to base." << std::endl;
            }
        }
    } else {
        std::cerr << "Game: Failed to get base tile coordinate for AI setup. Initial AI will idle or move randomly." << std::endl;
    }
    state = GameState::Playing1P; // 游戏初始化完成后进入游玩状态
}

void Game::setupLevel() {
    std::cout << "Setting up Level " << m_currentLevel << std::endl;
    state = GameState::LevelTransition; // 进入关卡过渡状态

    // 设置关卡转换时显示的文本
    m_levelTransitionMessageText.setString("Level " + std::to_string(m_currentLevel));
    sf::FloatRect textRect = m_levelTransitionMessageText.getLocalBounds();
    m_levelTransitionMessageText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
    float gameAreaWidth = 1200.f; // 游戏区域宽度
    m_levelTransitionMessageText.setPosition(gameAreaWidth / 2.0f, window.getSize().y / 2.0f);
    m_levelTransitionDisplayTimer = sf::seconds(2.5f);

    // =========================================================================
    // 关键步骤：清理上一关的实体，确保这里被正确执行
    // =========================================================================
    m_all_tanks.clear();       // <--- 清空所有现有坦克
    m_playerTankPtr = nullptr; // <--- 重置玩家坦克指针
    m_tools.clear();           // 清空所有道具

    // 重置子弹对象池中的所有子弹为不活动状态
    for(auto& bullet : m_bulletPool) {
        if(bullet) {
            bullet->setIsAlive(false);
        }
    }
    std::cout << "Entities from previous level (or existing ones) cleared." << std::endl;

    // 2. 生成新地图布局
    if (m_map.getMapWidth() <= 0 || m_map.getMapHeight() <= 0) {
        std::cerr << "CRITICAL ERROR in setupLevel: Map dimensions not properly set. Attempting to load them." << std::endl;
        if (!m_map.loadDimensionsAndTextures(*this)) {
            std::cerr << "CRITICAL ERROR in setupLevel: Failed to load map dimensions. Cannot proceed." << std::endl;
            window.close();
            return;
        }
    }
    m_map.generateLayout(m_currentLevel, m_rng, *this);
    m_map.resetForNewLevel();

    // 3. 重新创建/放置玩家坦克
    sf::Vector2f playerStartPos;
    bool spawnPointFound = false;

    std::vector<sf::Vector2i> preferredSpawnTiles;
    for (int y_offset = 0; y_offset < 3; ++y_offset) {
        for (int x_offset = 0; x_offset < 5; ++x_offset) {
            int spawnX = 1 + x_offset;
            int spawnY = m_map.getMapHeight() - 2 - y_offset;
            if (spawnX < m_map.getMapWidth() -1 && spawnY > 0) {
                preferredSpawnTiles.push_back(sf::Vector2i(spawnX, spawnY));
            }
        }
    }
    sf::Vector2i baseCoord = m_map.getBaseTileCoordinate();
    if (baseCoord.x != -1 && baseCoord.y != -1) {
        if(baseCoord.x - 2 > 0 && baseCoord.y - 2 > 0)
            preferredSpawnTiles.push_back(sf::Vector2i(baseCoord.x - 2, baseCoord.y - 2));
        if(baseCoord.x + 2 < m_map.getMapWidth() - 1 && baseCoord.y - 2 > 0)
            preferredSpawnTiles.push_back(sf::Vector2i(baseCoord.x + 2, baseCoord.y - 2));
    }

    for (const auto& tile : preferredSpawnTiles) {
        if (tile.x > 0 && tile.x < m_map.getMapWidth() - 1 &&
            tile.y > 0 && tile.y < m_map.getMapHeight() - 1 &&
            m_map.isTileWalkable(tile.x, tile.y)) {
            playerStartPos = sf::Vector2f(
                    static_cast<float>(tile.x * m_map.getTileWidth()) + m_map.getTileWidth() / 2.0f,
                    static_cast<float>(tile.y * m_map.getTileHeight()) + m_map.getTileHeight() / 2.0f
            );
            spawnPointFound = true;
            std::cout << "Player spawn point found at preferred tile (" << tile.x << ", " << tile.y << ")." << std::endl;
            break;
        }
    }

    if (!spawnPointFound) {
        std::cout << "Warning: Could not find preferred player spawn point. Searching broadly..." << std::endl;
        for (int y = m_map.getMapHeight() - 2; y > 0 && !spawnPointFound; --y) {
            for (int x = 1; x < m_map.getMapWidth() - 1 && !spawnPointFound; ++x) {
                if (m_map.isTileWalkable(x, y)) {
                    playerStartPos = sf::Vector2f(
                            static_cast<float>(x * m_map.getTileWidth()) + m_map.getTileWidth() / 2.0f,
                            static_cast<float>(y * m_map.getTileHeight()) + m_map.getTileHeight() / 2.0f
                    );
                    spawnPointFound = true;
                    std::cout << "Fallback player spawn point found at tile (" << x << ", " << y << ")." << std::endl;
                }
            }
        }
    }

    if (!spawnPointFound) {
        std::cerr << "CRITICAL ERROR: No walkable tile found for player spawn! Defaulting to top-left." << std::endl;
        playerStartPos = sf::Vector2f(
                static_cast<float>(m_map.getTileWidth() * 1.5f),
                static_cast<float>(m_map.getTileHeight() * 1.5f)
        );
    }

    auto player = std::make_unique<PlayerTank>(playerStartPos, Direction::UP, *this); // 创建新的玩家坦克
    m_playerTankPtr = player.get();                     // 更新指针
    m_all_tanks.push_back(std::move(player));           // 将新的玩家坦克添加到列表中
    std::cout << "Player tank recreated for Level " << m_currentLevel << " at pixel (" << playerStartPos.x << ", " << playerStartPos.y << ")." << std::endl;

    // 4. 重置AI和道具的生成计时器
    m_aiTankSpawnTimer = sf::Time::Zero;
    m_toolSpawnTimer = sf::Time::Zero;

    // 5. 根据关卡调整AI参数
    if (m_currentLevel == 1) {
        m_maxActiveAITanks = 5;
        m_aiTankSpawnInterval = sf::seconds(8.0f);
    } else if (m_currentLevel == 2) {
        m_maxActiveAITanks = 7;
        m_aiTankSpawnInterval = sf::seconds(6.5f);
    } else if (m_currentLevel >= 3) {
        m_maxActiveAITanks = 9;
        m_aiTankSpawnInterval = sf::seconds(5.0f);
    }
    std::cout << "AI parameters for Level " << m_currentLevel << ": MaxActive=" << m_maxActiveAITanks
              << ", SpawnInterval=" << m_aiTankSpawnInterval.asSeconds() << "s." << std::endl;

    // 6. AI目标更新 (如果有关卡开始时就存在的AI)
    if (baseCoord.x != -1 && baseCoord.y != -1) {
        for (const auto& tank_ptr : m_all_tanks) {
            if (AITank* ai = dynamic_cast<AITank*>(tank_ptr.get())) {
                if (tank_ptr.get() != m_playerTankPtr) { // 确保不是玩家自己
                    ai->setStrategicTargetTile(baseCoord);
                }
            }
        }
    }
}

void Game::advanceToNextLevel() {
    if (m_currentLevel < MAX_LEVEL) {
        m_currentLevel++;
        std::cout << "Advancing to Level " << m_currentLevel << std::endl;
        // 分数不清零，因为是总分判断
        setupLevel();
    } else {
        std::cout << "Congratulations! You have completed all levels!" << std::endl;
        state = GameState::GameOver; // 或者一个 GameState::GameWon
        m_levelTransitionMessageText.setString("YOU WIN!");
        sf::FloatRect textRect = m_levelTransitionMessageText.getLocalBounds();
        m_levelTransitionMessageText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
        m_levelTransitionMessageText.setPosition(window.getSize().x / 2.0f, window.getSize().y / 2.0f);
        m_levelTransitionDisplayTimer = sf::seconds(5.0f); // 显示胜利信息更长时间
    }
}

void Game::run() {
    while (window.isOpen()) {
        sf::Time deltaTime = clock.restart(); // 获取帧间隔时间

        Handling_events(deltaTime); // 处理事件
        if (state == GameState::Playing1P) { // 只有在游玩状态才更新游戏逻辑
            update(deltaTime);          // 更新游戏逻辑
        }
        render();                   // 渲染画面
    }
}

void Game::end() {
    std::cout << "Game::end() called." << std::endl;
    // 清理资源等 (如果需要，SFML资源通常会自动管理，unique_ptr也会自动释放)
}

// =========================================================================
// 内部核心逻辑方法
// =========================================================================
void Game::Handling_events(sf::Time dt) {
    sf::Event event{};
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window.close();
        }
        if (state == GameState::LevelTransition) continue; // 过渡时不处理游戏输入

        if (state == GameState::Playing1P) { // 只在游玩状态处理游戏输入
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Space) {
                    if (m_playerTankPtr && !m_playerTankPtr->isDestroyed() && m_playerTankPtr->canShoot()) {
                        m_playerTankPtr->shoot(*this);
                    }
                }
            }
        }
        // 任何状态下都可以处理的事件，例如R键重置整个游戏
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R) {
            if (state == GameState::GameOver || (m_playerTankPtr && m_playerTankPtr->isDestroyed() && m_currentLevel == 1 && score < SCORE_THRESHOLD_LEVEL_2 )) { // 游戏结束或第一关失败时可以重置
                std::cout << "Resetting game from beginning..." << std::endl;
                score = 0;
                m_currentLevel = 1; // 重置到第一关
                // init(); // 调用init会重新加载所有配置，可能有点重
                // 更轻量级的重置：
                setupLevel(); // 这会清理实体并设置第一关
                state = GameState::Playing1P; // 确保状态正确
                return;
            }
        }
    }

    // 处理玩家坦克的持续按键移动 (仅在游玩状态)
    if (state == GameState::Playing1P && m_playerTankPtr && !m_playerTankPtr->isDestroyed()) {
        float distance = m_playerTankPtr->getSpeed() * dt.asSeconds();
        bool moved = false;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
            m_playerTankPtr->setDirection(Direction::LEFT, *this);
            m_playerTankPtr->move(m_playerTankPtr->get_position() + sf::Vector2f(-distance, 0.f), m_map);
            moved = true;
        } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
            m_playerTankPtr->setDirection(Direction::RIGHT, *this);
            m_playerTankPtr->move(m_playerTankPtr->get_position() + sf::Vector2f(distance, 0.f), m_map);
            moved = true;
        } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
            m_playerTankPtr->setDirection(Direction::UP, *this);
            m_playerTankPtr->move(m_playerTankPtr->get_position() + sf::Vector2f(0.f, -distance), m_map);
            moved = true;
        } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
            m_playerTankPtr->setDirection(Direction::DOWN, *this);
            m_playerTankPtr->move(m_playerTankPtr->get_position() + sf::Vector2f(0.f, distance), m_map);
            moved = true;
        }
        // 如果没有移动，可以考虑停止动画或进入idle状态 (如果Tank类支持)
    }
}

void Game::update(sf::Time dt) {
        // 首先打印进入update时的状态和计时器信息
        // std::cout << "Game::update() called. Current state: " << static_cast<int>(state)
        //           << ", dt: " << dt.asSeconds()
        //           << ", TransitionTimer: " << m_levelTransitionDisplayTimer.asSeconds() << std::endl;

        // 1. 处理关卡过渡计时器和消息显示
        if (m_levelTransitionDisplayTimer > sf::Time::Zero) {
            m_levelTransitionDisplayTimer -= dt;
            // std::cout << "  TransitionTimer decremented. New value: " << m_levelTransitionDisplayTimer.asSeconds() << std::endl;

            if (m_levelTransitionDisplayTimer <= sf::Time::Zero) {
                m_levelTransitionDisplayTimer = sf::Time::Zero; // 确保不会变成负数
                std::cout << "  TransitionTimer reached zero." << std::endl;

                if (state == GameState::LevelTransition) {
                    state = GameState::Playing1P;
                    std::cout << "  STATE CHANGED: LevelTransition -> Playing1P for Level " << m_currentLevel << std::endl;
                } else if (state == GameState::GameOver) {
                    // 游戏结束信息显示完毕，可以保持 GameOver 状态，或者允许按键返回主菜单等
                    std::cout << "  GameOver message display finished. Game remains in GameOver state." << std::endl;
                    // 如果需要，可以在这里添加返回主菜单的逻辑，或者提示玩家按键
                }
                // 如果是 "YOU WIN!" 消息显示完毕，也会在这里，state 可能是 GameOver 或一个特定的 GameWon 状态
            }
        }

        // 2. 如果当前不是游玩状态，则不执行后续的游戏逻辑更新
        if (state != GameState::Playing1P) {
            // std::cout << "  Not in Playing1P state. Skipping main game logic update." << std::endl;
            return;
        }

        // std::cout << "  Executing main game logic for Playing1P state." << std::endl;

        // 3. 更新所有坦克的基础状态 (动画、通用计时器等)
        for(auto& tankPtr : m_all_tanks) {
            if(tankPtr && !tankPtr->isDestroyed()) {
                tankPtr->update(dt, *this);
            }
        }

        // 4. 更新AI坦克的特定逻辑 (移动决策、格子间移动、自动射击)
        for (auto& tankPtr : m_all_tanks) {
            if (tankPtr && !tankPtr->isDestroyed()) {
                if (AITank* aiTankPtr = dynamic_cast<AITank*>(tankPtr.get())) {
                    if (!aiTankPtr->isMoving()) {
                        aiTankPtr->decideNextAction(m_map, m_playerTankPtr);
                    }
                    aiTankPtr->updateMovementBetweenTiles(dt, m_map);

                    if (aiTankPtr->canShootAI()) {
                        aiTankPtr->shoot(*this);
                        aiTankPtr->resetShootTimerAI();
                    }
                }
            }
        }

        // 5. 处理坦克间的碰撞
        for (size_t i = 0; i < m_all_tanks.size(); ++i) {
            for (size_t j = i + 1; j < m_all_tanks.size(); ++j) {
                if (m_all_tanks[i] && !m_all_tanks[i]->isDestroyed() &&
                    m_all_tanks[j] && !m_all_tanks[j]->isDestroyed()) {
                    if (m_all_tanks[i]->getBounds().intersects(m_all_tanks[j]->getBounds())) {
                        resolveTankCollision(m_all_tanks[i].get(), m_all_tanks[j].get());
                    }
                }
            }
        }

        // 6. 更新所有活跃子弹的状态 (移动)
        for (auto& bullet_ptr : m_bulletPool) {
            if (bullet_ptr && bullet_ptr->isAlive()) {
                bullet_ptr->update(dt);
            }
        }

        // 7. 处理碰撞逻辑
        //    a. 子弹与坦克的碰撞
        for (auto& bullet_ptr : m_bulletPool) {
            if (bullet_ptr && bullet_ptr->isAlive()) {
                for (auto& tankPtr : m_all_tanks) {
                    if (tankPtr && !tankPtr->isDestroyed()) {
                        bool self_harm_scenario = false;
                        if (bullet_ptr->getType() == 1 && dynamic_cast<PlayerTank*>(tankPtr.get())) {
                            // self_harm_scenario = true; // 玩家子弹不伤玩家 (如果需要)
                        } else if (bullet_ptr->getType() == 2 && dynamic_cast<AITank*>(tankPtr.get())) {
                            // self_harm_scenario = true; // AI子弹不伤AI (如果需要)
                        }

                        if (!self_harm_scenario && bullet_ptr->getBounds().intersects(tankPtr->getBounds())) {
                            tankPtr->takeDamage(bullet_ptr->getDamage());
                            bullet_ptr->setIsAlive(false);
                            if (tankPtr->isDestroyed()) {
                                if (AITank* destroyedAI = dynamic_cast<AITank*>(tankPtr.get())) {
                                    score += destroyedAI->getScoreValue();
                                    std::cout << "AI Tank (type: " << destroyedAI->getTankType() << ") destroyed! Player Score: " << score << std::endl;
                                } else if (tankPtr.get() == m_playerTankPtr) {
                                    std::cout << "Player Tank destroyed by bullet!" << std::endl;
                                }
                            }
                            break;
                        }
                    }
                }
                if (!bullet_ptr->isAlive()) continue; // 如果子弹已被坦克碰撞处理，跳过与地图的碰撞

                // b. 子弹与地图的碰撞
                sf::Vector2f bulletPos = bullet_ptr->getPosition();
                if (m_map.getTileWidth() <= 0 || m_map.getTileHeight() <= 0) continue;
                int tileX = static_cast<int>(bulletPos.x / m_map.getTileWidth());
                int tileY = static_cast<int>(bulletPos.y / m_map.getTileHeight());

                if (tileX >= 0 && tileX < m_map.getMapWidth() && tileY >= 0 && tileY < m_map.getMapHeight()) {
                    int tileTypeHit = m_map.getTileType(tileX, tileY);
                    bool bulletHitWall = false;
                    if (tileTypeHit == 1) { // 砖墙
                        m_map.damageTile(tileX, tileY, 1, *this);
                        bulletHitWall = true;
                    } else if (tileTypeHit == 3) { // 基地
                        m_map.damageBase(bullet_ptr->getDamage());
                        bulletHitWall = true;
                    } else if (tileTypeHit == 2) { // 钢墙
                        bulletHitWall = true;
                    }
                    // 水(4)和森林(5)子弹可以穿过
                    if (bulletHitWall) {
                        bullet_ptr->setIsAlive(false);
                    }
                } else { // 子弹飞出地图边界
                    bullet_ptr->setIsAlive(false);
                }
            }
        }

        // 8. 更新道具逻辑 (生成、生命周期、碰撞)
        updateTools(dt);

        // 9. 更新AI坦克生成逻辑
        updateAITankSpawning(dt);

        // 10. 清理被摧毁的坦克
        m_all_tanks.erase(std::remove_if(m_all_tanks.begin(), m_all_tanks.end(),
                                         [&](const std::unique_ptr<Tank>& tank_to_check) {
                                             bool should_remove = tank_to_check && tank_to_check->isDestroyed();
                                             if (should_remove) {
                                                 if (tank_to_check.get() == m_playerTankPtr) {
                                                     m_playerTankPtr = nullptr; // 玩家坦克被移除，指针置空
                                                     std::cout << "Player tank pointer (m_playerTankPtr) set to nullptr after being destroyed and removed." << std::endl;
                                                 }
                                             }
                                             return should_remove;
                                         }),
                          m_all_tanks.end());

        // 11. 检查游戏结束条件 (在清理坦克之后)
        if (!m_playerTankPtr && state == GameState::Playing1P) { // 玩家坦克指针为空且之前在游玩状态
            std::cout << "Game Over! Player Tank was destroyed and removed from game." << std::endl;
            state = GameState::GameOver;
            m_levelTransitionMessageText.setString("GAME OVER\nPlayer Destroyed!\nPress 'R' to Restart");
            sf::FloatRect textRect = m_levelTransitionMessageText.getLocalBounds(); // 重新获取边界以居中
            m_levelTransitionMessageText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
            m_levelTransitionMessageText.setPosition(1200.f / 2.0f, window.getSize().y / 2.0f); // 游戏区域中心
            m_levelTransitionDisplayTimer = sf::seconds(10.0f); // 持续显示Game Over信息
        }
        if (m_map.isBaseDestroyed() && state == GameState::Playing1P) { // 基地被摧毁且之前在游玩状态
            std::cout << "Game Over! Base was destroyed." << std::endl;
            state = GameState::GameOver;
            m_levelTransitionMessageText.setString("GAME OVER\nBase Destroyed!\nPress 'R' to Restart");
            sf::FloatRect textRect = m_levelTransitionMessageText.getLocalBounds();
            m_levelTransitionMessageText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
            m_levelTransitionMessageText.setPosition(1200.f / 2.0f, window.getSize().y / 2.0f);
            m_levelTransitionDisplayTimer = sf::seconds(10.0f);
        }

        // 12. 检查关卡晋级条件 (只有在Playing1P状态且游戏未结束时)
        if (state == GameState::Playing1P) { // 确保在游玩状态才检查晋级
            bool advanced = false;
            if (m_currentLevel == 1 && score >= SCORE_THRESHOLD_LEVEL_2) {
                advanceToNextLevel();
                advanced = true;
            } else if (m_currentLevel == 2 && score >= SCORE_THRESHOLD_LEVEL_3) {
                advanceToNextLevel();
                advanced = true;
            }
            // 如果晋级了，advanceToNextLevel 内部会将 state 设置为 LevelTransition，
            // 那么本轮 update 后续的 Playing1P 逻辑就不应该再执行了。
            // advanceToNextLevel 内部调用 setupLevel，setupLevel 会设置 LevelTransition 状态。
            if (advanced) {
                std::cout << "Level advanced. Current state should be LevelTransition. Skipping rest of Playing1P update." << std::endl;
                return; // 如果已晋级，则提前结束本次update，等待下一帧处理LevelTransition
            }
        }

        // 如果游戏结束，可以执行一些清理或状态转换 (这部分逻辑主要由上面的计时器和状态转换处理)
        // if (state == GameState::GameOver) {
        //     // std::cout << "Final Score: " << score << std::endl;
        // }
    }

void Game::render() {
    window.clear(sf::Color(100, 100, 100)); // 清屏，使用深灰色背景

    // 绘制地图 (游戏区域)
    m_map.draw(window, *this);

    // 绘制所有坦克 (游戏区域)
    for (const auto &tank: m_all_tanks) {
        if (tank && !tank->isDestroyed()) {
            tank->draw(window);
        }
    }

    // 绘制所有活跃子弹 (游戏区域)
    for (const auto &bullet_ptr: m_bulletPool) {
        if (bullet_ptr && bullet_ptr->isAlive()) {
            bullet_ptr->draw(window);
        }
    }

    // 绘制所有活动道具 (游戏区域)
    for (const auto &tool : m_tools) {
        if (tool && tool->isActive()) {
            tool->draw(window);
        }
    }

    // --- 开始绘制右侧UI面板 (1200px 至 1500px) ---
    float uiPanelX = 1200.f; // UI面板的起始X坐标
    float currentY = 30.f;   // UI元素的当前Y坐标
    float lineSpacing = 28.f; // 每行文本的垂直间距
    float indentX = uiPanelX + 15.f; // UI文本的X坐标 (带一点左边距)
    float statsIndentX = indentX + 10.f; // 玩家具体属性的缩进X坐标

    // 1. 当前关卡
    m_currentLevelText.setString("Level: " + std::to_string(m_currentLevel));
    m_currentLevelText.setPosition(indentX, currentY);
    window.draw(m_currentLevelText);
    currentY += lineSpacing * 1.2f; // 稍大间距

    // 2. 基地血量
    m_baseHealthText.setString("Base HP: " + std::to_string(m_map.getBaseHealth()));
    m_baseHealthText.setPosition(indentX, currentY);
    window.draw(m_baseHealthText);
    currentY += lineSpacing * 1.2f;

    // 3. 总分数
    m_scoreText.setString("Score: " + std::to_string(score));
    m_scoreText.setPosition(indentX, currentY);
    window.draw(m_scoreText);
    currentY += lineSpacing * 1.8f; // 较大间距，分隔玩家状态

    // 4. 玩家Tank当前数值
    if (m_playerTankPtr && !m_playerTankPtr->isDestroyed()) {
        m_playerStatsTitleText.setString("Player Stats:"); // 玩家状态标题
        m_playerStatsTitleText.setPosition(indentX - 5.f, currentY); // 标题稍微突出
        window.draw(m_playerStatsTitleText);
        currentY += lineSpacing * 1.3f; // 标题后的间距

        // 玩家HP
        m_playerHealthText.setString("HP: " + std::to_string(m_playerTankPtr->getHealth()) + "/" + std::to_string(m_playerTankPtr->getMaxHealth()));
        m_playerHealthText.setPosition(statsIndentX, currentY);
        window.draw(m_playerHealthText);
        currentY += lineSpacing;

        // 玩家护甲
        m_playerArmorText.setString("Armor: " + std::to_string(m_playerTankPtr->getArmor()));
        m_playerArmorText.setPosition(statsIndentX, currentY);
        window.draw(m_playerArmorText);
        currentY += lineSpacing;

        // 玩家攻击力
        m_playerAttackText.setString("Attack: " + std::to_string(m_playerTankPtr->getCurrentAttackPower()));
        m_playerAttackText.setPosition(statsIndentX, currentY);
        window.draw(m_playerAttackText);
        currentY += lineSpacing;

        // 玩家速度 (格式化为一位小数)
        std::ostringstream speedStream;
        speedStream << std::fixed << std::setprecision(1) << m_playerTankPtr->getSpeed();
        m_playerSpeedText.setString("Speed: " + speedStream.str());
        m_playerSpeedText.setPosition(statsIndentX, currentY);
        window.draw(m_playerSpeedText);
        currentY += lineSpacing;

        // 玩家射击冷却 (格式化为两位小数)
        std::ostringstream cooldownStream;
        cooldownStream << std::fixed << std::setprecision(2) << m_playerTankPtr->getShootCooldown().asSeconds();
        m_playerCooldownText.setString("Cooldown: " + cooldownStream.str() + "s");
        m_playerCooldownText.setPosition(statsIndentX, currentY);
        window.draw(m_playerCooldownText);
        // currentY += lineSpacing; // 这是玩家状态的最后一项，后面没有其他常规UI项了

    } else if (state == GameState::Playing1P || state == GameState::GameOver || state == GameState::LevelTransition) {
        // 如果玩家坦克不存在 (例如被摧毁了)，或者在特定状态下
        m_playerDestroyedText.setString("Player Destroyed!");
        m_playerDestroyedText.setPosition(indentX, currentY); // 显示在玩家状态区域
        window.draw(m_playerDestroyedText);
    }
    // --- UI面板绘制结束 ---


    // 绘制关卡切换信息、游戏结束信息或胜利信息 (如果计时器激活)
    // 这些信息通常显示在屏幕中央
    if (m_levelTransitionDisplayTimer > sf::Time::Zero) {
        // 确保文本居中
        sf::FloatRect textBounds = m_levelTransitionMessageText.getLocalBounds();
        m_levelTransitionMessageText.setOrigin(textBounds.left + textBounds.width / 2.0f, textBounds.top + textBounds.height / 2.0f);
        // 位置设置在游戏区域的中心 (0到uiPanelX之间)
        m_levelTransitionMessageText.setPosition( (uiPanelX / 2.0f), window.getSize().y / 2.0f);
        window.draw(m_levelTransitionMessageText);
    }

    window.display(); // 显示所有绘制的内容
}


// =========================================================================
// 内部初始化与加载方法
// =========================================================================
bool Game::loadConfig(const std::string& configPath) {
    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
        std::cerr << "CRITICAL ERROR: Failed to open config file: " << configPath << std::endl;
        return false;
    }

    try {
        configFile >> m_configJson;
        std::cout << "Config file '" << configPath << "' loaded and parsed successfully." << std::endl;

        // --- 加载道具纹理 ---
        if (m_configJson.contains("textures") && m_configJson["textures"].contains("props")) {
            for (auto& [key, pathNode] : m_configJson["textures"]["props"].items()) {
                if (pathNode.is_string()) {
                    std::string path = pathNode.get<std::string>();
                    if (!loadTextureFromJson(key, path)) {
                        std::cerr << "Warning: Failed to load prop texture '" << key << "' from path: " << path << std::endl;
                    } else {
                        std::cout << "Loaded prop texture: " << key << std::endl;
                    }
                }
            }
        } else { std::cerr << "Warning: 'textures.props' not found in config." << std::endl;}

        // --- 加载地图瓦片纹理 ---
        if (m_configJson.contains("textures") && m_configJson["textures"].contains("map_tiles")) {
            for (auto& [key, pathNode] : m_configJson["textures"]["map_tiles"].items()) {
                if (pathNode.is_string()) {
                    std::string path = pathNode.get<std::string>();
                    if (!loadTextureFromJson("map_" + key, path)) { // 给地图瓦片键名加上 "map_" 前缀
                        std::cerr << "Warning: Failed to load map_tile texture '" << key << "' from path: " << path << std::endl;
                    } else {
                        std::cout << "Loaded map_tile texture: map_" << key << std::endl;
                    }
                }
            }
        } else { std::cerr << "Warning: 'textures.map_tiles' not found in config." << std::endl;}

        // --- 加载坦克纹理 (支持多帧动画) ---
        if (m_configJson.contains("textures") && m_configJson["textures"].contains("tanks")) {
            for (auto& [tankType, directionsNode] : m_configJson["textures"]["tanks"].items()) {
                std::cout << "Loading textures for tank type: " << tankType << std::endl;
                for (auto& [dirStr, pathsNode] : directionsNode.items()) {
                    Direction dirEnum;
                    if (dirStr == "up") dirEnum = Direction::UP;
                    else if (dirStr == "down") dirEnum = Direction::DOWN;
                    else if (dirStr == "left") dirEnum = Direction::LEFT;
                    else if (dirStr == "right") dirEnum = Direction::RIGHT;
                    else {
                        std::cerr << "Warning: Unknown direction string '" << dirStr << "' for tank type '" << tankType << "'" << std::endl;
                        continue;
                    }

                    std::vector<sf::Texture> frameTextures;
                    if (pathsNode.is_array()) { // 多帧动画
                        for (const auto& pathNodeFrame : pathsNode) {
                            if(pathNodeFrame.is_string()){
                                sf::Texture tempTexture;
                                std::string path = pathNodeFrame.get<std::string>();
                                if (tempTexture.loadFromFile(path)) {
                                    frameTextures.push_back(tempTexture);
                                } else {
                                    std::cerr << "Warning: Failed to load tank texture frame for type '" << tankType << "', dir '" << dirStr << "' from path: " << path << std::endl;
                                }
                            }
                        }
                    } else if (pathsNode.is_string()) { // 单帧纹理
                        sf::Texture singleTexture;
                        std::string path = pathsNode.get<std::string>();
                        if (singleTexture.loadFromFile(path)) {
                            frameTextures.push_back(singleTexture);
                        } else {
                            std::cerr << "Warning: Failed to load single tank texture for type '" << tankType << "', dir '" << dirStr << "' from path: " << path << std::endl;
                        }
                    }
                    if (!frameTextures.empty()) {
                        m_tankTextureCache[tankType][dirEnum] = frameTextures;
                        std::cout << "  Loaded " << frameTextures.size() << " frames for " << tankType << " - " << dirStr << std::endl;
                    }
                }
            }
        } else { std::cerr << "Warning: 'textures.tanks' not found in config." << std::endl;}


        // --- 加载子弹纹理 ---
        if (m_configJson.contains("textures") && m_configJson["textures"].contains("bullets")) {
            for (auto& [dirStr, pathNode] : m_configJson["textures"]["bullets"].items()) {
                if(pathNode.is_string()){
                    std::string path = pathNode.get<std::string>();
                    std::string bulletKey = "bullet_" + dirStr; // 例如: "bullet_up"
                    if (!loadTextureFromJson(bulletKey, path)) {
                        std::cerr << "Warning: Failed to load bullet texture for direction '" << dirStr << "' from path: " << path << std::endl;
                    } else {
                        std::cout << "Loaded bullet texture: " << bulletKey << std::endl;
                    }
                }
            }
        } else { std::cerr << "Warning: 'textures.bullets' not found in config." << std::endl;}


    } catch (nlohmann::json::parse_error& e) {
        std::cerr << "CRITICAL ERROR: JSON parsing failed: " << e.what() << std::endl;
        return false;
    } catch (nlohmann::json::type_error& e) {
        std::cerr << "CRITICAL ERROR: JSON type error: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "CRITICAL ERROR: An unexpected error occurred during config loading: " << e.what() << std::endl;
        return false;
    }
    return true;
}

bool Game::loadTextureFromJson(const std::string& key, const std::string& path) {
    sf::Texture texture;
    if (!texture.loadFromFile(path)) {
        // 错误信息已在调用处打印，这里可以不再重复打印，或者打印更简洁的
        // std::cerr << "Game::loadTextureFromJson - Error loading texture '" << key << "' from: " << path << std::endl;
        return false;
    }
    m_textureCache[key] = texture;
    return true;
}

void Game::loadToolTypesFromConfig() {
    m_availableToolTypes.clear();
    if (m_configJson.contains("textures") && m_configJson["textures"].contains("props")) {
        for (auto& [key, pathNode] : m_configJson["textures"]["props"].items()) {
            // 只需要键名，路径在加载纹理时已使用
            m_availableToolTypes.push_back(key);
            std::cout << "Found available tool type from config: " << key << std::endl;
        }
    }
    if (m_availableToolTypes.empty()) {
        std::cerr << "Warning: No tool types found in config.json under textures.props. No tools will be spawned." << std::endl;
    }
}

void Game::loadAITankConfigs() {
    m_aiTypeConfigs.clear();
    m_availableAITankTypeNames.clear();

    if (m_configJson.contains("ai_settings") && m_configJson["ai_settings"].contains("ai_types")) {
        const auto& aiTypesNode = m_configJson["ai_settings"]["ai_types"];
        for (auto it = aiTypesNode.begin(); it != aiTypesNode.end(); ++it) {
            const std::string& typeName = it.key();
            const auto& configNode = it.value();

            try {
                AITankTypeConfig config;
                config.typeName = typeName;
                // 从 "ai_settings" 的全局默认值开始，如果特定类型没有定义，则使用全局默认
                // 如果全局默认也没有，则使用Game类中硬编码的m_defaultAI...值
                config.baseHealth = configNode.value("base_health", m_configJson["ai_settings"].value("default_base_health", m_defaultAIBaseHealth));
                config.baseSpeed = configNode.value("base_speed", m_configJson["ai_settings"].value("default_base_speed", m_defaultAITankSpeed));
                config.baseAttack = configNode.value("base_attack", m_configJson["ai_settings"].value("default_base_attack", m_defaultAIBaseAttack));
                config.frameWidth = configNode.value("frame_width", m_configJson["ai_settings"].value("default_frame_width", m_defaultAIFrameWidth));
                config.frameHeight = configNode.value("frame_height", m_configJson["ai_settings"].value("default_frame_height", m_defaultAIFrameHeight));
                config.scoreValue = configNode.value("score_value", m_configJson["ai_settings"].value("default_score_value", m_defaultAIScoreValue));
                config.textureKey = configNode.value("texture_key", typeName); // 默认纹理键名与AI类型名一致

                m_aiTypeConfigs[typeName] = config;
                m_availableAITankTypeNames.push_back(typeName);
                std::cout << "Loaded AI Tank Config: " << typeName << " (HP:" << config.baseHealth << ", Speed:" << config.baseSpeed << ", Attack:" << config.baseAttack << ", Score:" << config.scoreValue << ")" << std::endl;
            } catch (const nlohmann::json::exception& e) {
                std::cerr << "Error parsing AI type config for '" << typeName << "': " << e.what() << std::endl;
            }
        }
    } else {
        std::cerr << "Warning: 'ai_settings.ai_types' not found in config.json. No specific AI types loaded." << std::endl;
    }

    if (m_availableAITankTypeNames.empty()) {
        std::cerr << "CRITICAL: No AI tank types available to spawn! Check 'config.json' for 'ai_settings.ai_types'." << std::endl;
        // 此时游戏可能无法正常生成AI坦克
    }
}

void Game::initializeBulletPool() {
    m_bulletPool.clear();
    m_bulletPool.reserve(INITIAL_BULLET_POOL_SIZE);

    const sf::Texture& defaultBulletTexture = getTexture("bullet_up"); // 获取一个默认的子弹纹理
    if (defaultBulletTexture.getSize().x == 0 || defaultBulletTexture.getSize().y == 0) {
        std::cerr << "CRITICAL ERROR: Default bullet texture ('bullet_up') is not loaded or invalid. Cannot pre-allocate bullet pool." << std::endl;
        return;
    }

    std::cout << "Pre-allocating bullet pool with " << INITIAL_BULLET_POOL_SIZE << " bullets..." << std::endl;
    for (size_t i = 0; i < INITIAL_BULLET_POOL_SIZE; ++i) {
        auto bullet = std::make_unique<Bullet>(
                defaultBulletTexture, sf::Vector2f(0.f, 0.f), Direction::UP, sf::Vector2f(0.f, -1.f),
                0, 0.f, 0 // 伤害, 速度, 类型 (占位符)
        );
        bullet->setIsAlive(false); // 新创建的池对象必须是不活跃的
        m_bulletPool.push_back(std::move(bullet));
    }
    std::cout << "Bullet pool pre-allocated. Size: " << m_bulletPool.size() << std::endl;
}

// =========================================================================
// Getter 方法 - 资源访问
// =========================================================================
const sf::Texture& Game::getTexture(const std::string& key) const {
    auto it = m_textureCache.find(key);
    if (it != m_textureCache.end()) {
        return it->second;
    }
    static sf::Texture emptyTexture; // 静态空纹理，避免每次都创建
    std::cerr << "Game::getTexture() Error: Texture with key '" << key << "' not found in cache. Returning empty texture." << std::endl;
    return emptyTexture;
}

const std::vector<sf::Texture>& Game::getTankTextures(const std::string& tankType, Direction dir) const {
    auto typeIt = m_tankTextureCache.find(tankType);
    if (typeIt != m_tankTextureCache.end()) {
        auto dirIt = typeIt->second.find(dir);
        if (dirIt != typeIt->second.end()) {
            return dirIt->second; // 返回找到的纹理帧列表
        }
    }
    static std::vector<sf::Texture> emptyTankTextures; // 静态空列表
    std::cerr << "Game::getTankTextures() Error: Tank textures not found for type '" << tankType
              << "' and direction " << static_cast<int>(dir) << ". Returning empty vector." << std::endl;
    return emptyTankTextures;
}

// =========================================================================
// Getter 方法 - 游戏状态与对象访问
// =========================================================================
Bullet* Game::getAvailableBullet() {
    for (const auto& bullet_ptr : m_bulletPool) {
        if (bullet_ptr && !bullet_ptr->isAlive()) {
            // std::cout << "Reusing bullet from pool." << std::endl;
            return bullet_ptr.get(); // 返回一个不活跃的子弹指针供复用
        }
    }

    // 如果池中所有子弹都在使用中，则动态创建一个新的 (如果允许池扩展)
    // 注意：动态扩展池可能会导致性能波动，最好预分配足够大的池
    // std::cout << "Bullet pool fully active, creating a new bullet instance (pool size: " << m_bulletPool.size() << ")." << std::endl;
    const sf::Texture& defaultBulletTexture = getTexture("bullet_up");
    if (defaultBulletTexture.getSize().x == 0) {
        std::cerr << "CRITICAL ERROR: Default bullet texture for new bullet is invalid in getAvailableBullet!" << std::endl;
        return nullptr;
    }
    auto new_bullet = std::make_unique<Bullet>(
            defaultBulletTexture, sf::Vector2f(0.f, 0.f), Direction::UP, sf::Vector2f(0.f, -1.f), 0, 0.f, 0
    );
    new_bullet->setIsAlive(false); // 新创建的也先设为不活跃，让调用者reset并激活
    Bullet* raw_ptr = new_bullet.get();
    m_bulletPool.push_back(std::move(new_bullet)); // 添加到池中
    // std::cout << "New bullet added to pool. Pool size now: " << m_bulletPool.size() << std::endl;
    return raw_ptr;
}

// =========================================================================
// 碰撞处理方法
// =========================================================================
void Game::resolveTankCollision(Tank* tank1, Tank* tank2) {
    if (!tank1 || !tank2 || tank1->isDestroyed() || tank2->isDestroyed()) return;

    sf::FloatRect bounds1 = tank1->getBounds();
    sf::FloatRect bounds2 = tank2->getBounds();
    sf::FloatRect intersection;

    if (bounds1.intersects(bounds2, intersection)) {
        sf::Vector2f pos1 = tank1->get_position();
        sf::Vector2f pos2 = tank2->get_position();

        sf::Vector2f pushDirection = pos1 - pos2; // 推离方向
        if (pushDirection.x == 0.f && pushDirection.y == 0.f) { // 完全重叠
            pushDirection = sf::Vector2f(0.f, -1.f); // 默认向上推开
        }

        float length = std::sqrt(pushDirection.x * pushDirection.x + pushDirection.y * pushDirection.y);
        if (length != 0.f) {
            pushDirection /= length; // 归一化
        }

        // 推开的幅度应略大于重叠深度的一半，以确保分开
        float pushMagnitude = (std::min(intersection.width, intersection.height) / 2.0f) + 1.0f; // 增加一点余量
        sf::Vector2f moveOffset = pushDirection * pushMagnitude;

        // 尝试移动坦克，Tank::move会进行地图碰撞检测
        // 注意：这里的move调用不应该直接修改m_position，而是尝试移动。
        // Tank::move本身会更新m_position如果移动成功。
        tank1->move(pos1 + moveOffset, m_map);
        tank2->move(pos2 - moveOffset, m_map);


        // AI坦克碰撞后可能需要重新规划路径
        AITank* ai1 = dynamic_cast<AITank*>(tank1);
        AITank* ai2 = dynamic_cast<AITank*>(tank2);
        // if (ai1 && ai1->isMoving()) { /* ai1->forceReplanPath(); */ } // 示例：强制AI重新规划
        // if (ai2 && ai2->isMoving()) { /* ai2->forceReplanPath(); */ }
    }
}

void Game::resolveTankToolCollision(Tank* tank, Tools* tool) {
    if (!tank || tank->isDestroyed() || !tool || !tool->isActive()) {
        return;
    }
    // 当前设计：任何坦克都可以拾取道具
    // PlayerTank* player = dynamic_cast<PlayerTank*>(tank);
    // if (player) { // 如果只想让玩家拾取
    std::cout << "Tank (type: " << tank->getTankType() << ") collided with tool. Applying effect." << std::endl;
    tool->applyEffect(*tank, *this); // 调用道具的 applyEffect
    // 道具的 setActive(false) 应该在其 applyEffect 方法中调用来标记为已使用
    // }
}

// =========================================================================
// 游戏对象生成与管理方法
// =========================================================================

void Game::spawnRandomTool() {
    if (m_availableToolTypes.empty()) {
        // std::cout << "No available tool types to spawn." << std::endl;
        return;
    }
    if (m_tools.size() >= 5) { // 限制屏幕上最多同时存在的道具数量
        // std::cout << "Maximum number of tools (" << m_tools.size() << ") reached on map. Skipping spawn." << std::endl;
        return;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distribType(0, m_availableToolTypes.size() - 1);
    std::string randomToolKey = m_availableToolTypes[distribType(gen)];

    sf::Vector2f spawnPosition;
    bool positionFound = false;
    int maxAttempts = 100;
    int tileW = m_map.getTileWidth();
    int tileH = m_map.getTileHeight();

    if (tileW <= 0 || tileH <= 0) {
        std::cerr << "spawnRandomTool Error: Invalid tile dimensions from map (W:" << tileW << ", H:" << tileH << ")." << std::endl;
        return;
    }

    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        std::uniform_int_distribution<> distribX(1, m_map.getMapWidth() - 2);  // 避开最外层边界
        std::uniform_int_distribution<> distribY(1, m_map.getMapHeight() - 2); // 避开最外层边界
        int tileX = distribX(gen);
        int tileY = distribY(gen);

        if (m_map.isTileWalkable(tileX, tileY)) {
            // 简单检查该位置是否已有道具 (基于瓦片中心)
            sf::FloatRect newToolProspectiveBounds(
                    static_cast<float>(tileX * tileW), static_cast<float>(tileY * tileH),
                    static_cast<float>(tileW), static_cast<float>(tileH)
            );
            bool toolAlreadyThere = false;
            for (const auto& existingTool : m_tools) {
                if (existingTool && existingTool->isActive() && existingTool->getBound().intersects(newToolProspectiveBounds)) {
                    toolAlreadyThere = true;
                    break;
                }
            }
            if (!toolAlreadyThere) {
                spawnPosition = sf::Vector2f( // 道具放在瓦片中心
                        static_cast<float>(tileX * tileW) + tileW / 2.0f,
                        static_cast<float>(tileY * tileH) + tileH / 2.0f
                );
                positionFound = true;
                break;
            }
        }
    }

    if (!positionFound) {
        // std::cout << "spawnRandomTool: Could not find a suitable walkable tile to spawn a tool after " << maxAttempts << " attempts." << std::endl;
        return;
    }

    const sf::Texture& toolTexture = getTexture(randomToolKey);
    if (toolTexture.getSize().x == 0 || toolTexture.getSize().y == 0) {
        std::cerr << "spawnRandomTool Error: Failed to get texture for tool key '" << randomToolKey << "'" << std::endl;
        return;
    }

    std::unique_ptr<Tools> newTool = nullptr;
    if (randomToolKey == "add_armor") newTool = std::make_unique<AddArmor>(spawnPosition, toolTexture);
    else if (randomToolKey == "add_attack") newTool = std::make_unique<AddAttack>(spawnPosition, toolTexture);
    else if (randomToolKey == "add_attack_speed") newTool = std::make_unique<AddAttackSpeed>(spawnPosition, toolTexture);
    else if (randomToolKey == "add_speed") newTool = std::make_unique<AddSpeed>(spawnPosition, toolTexture);
    else if (randomToolKey == "grenade") newTool = std::make_unique<GrenadeTool>(spawnPosition, toolTexture);
    else if (randomToolKey == "slow_down_ai") newTool = std::make_unique<SlowDownAI>(spawnPosition, toolTexture);
    else {
        std::cerr << "spawnRandomTool Error: Unknown tool key '" << randomToolKey << "'" << std::endl;
        return;
    }

    if (newTool) {
        m_tools.push_back(std::move(newTool));
        std::cout << "Spawned tool '" << randomToolKey << "' at (" << spawnPosition.x << ", " << spawnPosition.y << ")" << std::endl;
    }
}

void Game::updateTools(sf::Time dt) {
    m_toolSpawnTimer += dt;
    if (m_toolSpawnTimer >= m_toolSpawnInterval) {
        spawnRandomTool();
        m_toolSpawnTimer = sf::Time::Zero; // 重置计时器
    }

    for (auto& tool : m_tools) { // 更新所有活动道具 (例如生命周期)
        if (tool && tool->isActive()) {
            tool->update(dt);
        }
    }


    // 处理坦克与道具的碰撞
    for (auto& tankPtr : m_all_tanks) {
        if (tankPtr && !tankPtr->isDestroyed()) {
            for (auto& toolPtr : m_tools) {
                if (toolPtr && toolPtr->isActive()) {
                    if (tankPtr->getBounds().intersects(toolPtr->getBound())) {
                        resolveTankToolCollision(tankPtr.get(), toolPtr.get());
                        if (!toolPtr->isActive()) break; // 如果道具已失效，跳出内层循环
                    }
                }
            }
        }
    }

    // 清理不再活动的道具
    m_tools.erase(std::remove_if(m_tools.begin(), m_tools.end(),
                                 [](const std::unique_ptr<Tools>& t) {
                                     return !t || !t->isActive();
                                 }),
                  m_tools.end());
}

void Game::spawnNewAITank() {
    int currentAICount = 0;
    for(const auto& tank : m_all_tanks){
        if(dynamic_cast<AITank*>(tank.get()) && !tank->isDestroyed()){
            currentAICount++;
        }
    }
    if (currentAICount >= m_maxActiveAITanks) {
        // std::cout << "Max AI tank limit reached (" << currentAICount << "). Skipping AI spawn." << std::endl;
        return;
    }

    if (m_availableAITankTypeNames.empty()) {
        std::cerr << "spawnNewAITank: No AI types loaded from config. Cannot spawn AI." << std::endl;
        return;
    }

    std::random_device rd_type;
    std::mt19937 gen_type(rd_type());
    std::uniform_int_distribution<> distrib_type_idx(0, m_availableAITankTypeNames.size() - 1);
    std::string selectedTypeName = m_availableAITankTypeNames[distrib_type_idx(gen_type)];

    const AITankTypeConfig* selectedConfig = nullptr;
    auto configIt = m_aiTypeConfigs.find(selectedTypeName);
    if (configIt != m_aiTypeConfigs.end()) {
        selectedConfig = &configIt->second;
    } else {
        std::cerr << "spawnNewAITank: Could not find config for selected AI type '" << selectedTypeName << "'. Aborting spawn." << std::endl;
        return;
    }

    sf::Vector2f spawnPosition;
    bool positionFound = false;
    int maxAttempts = 50;
    int tileW = m_map.getTileWidth();
    int tileH = m_map.getTileHeight();

    if (tileW <= 0 || tileH <= 0) {
        std::cerr << "spawnNewAITank Error: Invalid tile dimensions from map. Cannot determine spawn position." << std::endl;
        return;
    }

    // AI 出生区域：地图上半部分，避开边界
    int minYTile = 1;
    int maxYTile = std::max(minYTile + 1, m_map.getMapHeight() / 2); // 至少是 minYTile + 1
    maxYTile = std::min(maxYTile, m_map.getMapHeight() - 2); // 不超过倒数第二行

    std::random_device rd_pos;
    std::mt19937 gen_pos(rd_pos());

    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        std::uniform_int_distribution<> distribX(1, m_map.getMapWidth() - 2);
        std::uniform_int_distribution<> distribY(minYTile, maxYTile);
        int tileX = distribX(gen_pos);
        int tileY = distribY(gen_pos);

        if (m_map.isTileWalkable(tileX, tileY)) {
            sf::Vector2f prospectiveCenter(
                    static_cast<float>(tileX * tileW) + tileW / 2.0f,
                    static_cast<float>(tileY * tileH) + tileH / 2.0f
            );
            // 简单检查该位置是否已有坦克 (基于大致距离)
            bool tankAlreadyThere = false;
            float minDistanceSq = static_cast<float>((tileW * 0.9f) * (tileW * 0.9f)); // 坦克间最小距离平方
            for (const auto& existingTank : m_all_tanks) {
                if (existingTank && !existingTank->isDestroyed()) {
                    sf::Vector2f diff = existingTank->get_position() - prospectiveCenter;
                    if ((diff.x * diff.x + diff.y * diff.y) < minDistanceSq) {
                        tankAlreadyThere = true;
                        break;
                    }
                }
            }
            if (!tankAlreadyThere) {
                spawnPosition = prospectiveCenter;
                positionFound = true;
                break;
            }
        }
    }

    if (!positionFound) {
        // std::cout << "spawnNewAITank: Could not find a suitable spawn location after " << maxAttempts << " attempts." << std::endl;
        return;
    }

    std::uniform_int_distribution<> distribDir(0, 3); // 0:UP, 1:DOWN, 2:LEFT, 3:RIGHT (与Direction枚举顺序可能不同，需要映射)
    Direction startDir = static_cast<Direction>(distribDir(gen_pos)); // 假设枚举值与0-3对应

    auto newAITank = std::make_unique<AITank>(
            spawnPosition, startDir, selectedConfig->typeName, *this,
            selectedConfig->baseSpeed, selectedConfig->baseHealth, selectedConfig->baseAttack,
            selectedConfig->frameWidth, selectedConfig->frameHeight, selectedConfig->scoreValue
    );

    AITank* aiPtr = newAITank.get();
    m_all_tanks.push_back(std::move(newAITank));

    if (aiPtr) {
        sf::Vector2i baseTile = m_map.getBaseTileCoordinate();
        if (baseTile.x != -1 && baseTile.y != -1) {
            aiPtr->setStrategicTargetTile(baseTile);
        } else {
            std::cerr << "  spawnNewAITank: Could not set target for new AI tank (type: " << aiPtr->getTankType() << "): Base tile not found." << std::endl;
        }
        std::cout << "Spawned AI Tank (type: " << aiPtr->getTankType() << ") at (" << spawnPosition.x << ", " << spawnPosition.y << "). Target set to base." << std::endl;
    } else {
        std::cerr << "spawnNewAITank: Failed to create new AITank instance for type '" << selectedConfig->typeName << "'." << std::endl;
    }
}

void Game::updateAITankSpawning(sf::Time dt) {
    m_aiTankSpawnTimer += dt;
    if (m_aiTankSpawnTimer >= m_aiTankSpawnInterval) {
        spawnNewAITank();
        m_aiTankSpawnTimer = sf::Time::Zero; // 重置计时器
    }
}
