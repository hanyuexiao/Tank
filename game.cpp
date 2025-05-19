#include "game.h"
#include "AITank.h"
#include "PlayerTank.h"
#include "Bullet.h"
#include "Map.h"
// 确保所有道具的头文件都被包含
#include "Tools.h" // 确保 Tools.h 在具体道具类之前
#include "AddArmor.h"
#include "AddAttack.h"
#include "AddAttackSpeed.h"
#include "AddSpeed.h"
#include "Grenade.h"
#include "SlowDownAI.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <random> // 用于随机数

// Game 类的构造函数
Game::Game(): window(sf::VideoMode(1500, 750), "Tank Battle!"),
              state(GameState::MainMenu),
              score(0),
              life(3),
              m_map(),
              m_playerTankPtr(nullptr), // 确保初始化 PlayerTank 指针
              m_toolSpawnInterval(sf::seconds(10.0f)), // 初始化道具生成间隔为10秒
              m_toolSpawnTimer(sf::Time::Zero),        // 初始化道具生成计时器
              m_aiTankSpawnInterval(sf::seconds(5.0f)),   // 初始化AI坦克生成间隔为5秒 (会被config覆盖)
              m_aiTankSpawnTimer(sf::Time::Zero),         // 初始化AI坦克生成计时器
              m_maxActiveAITanks(10),                     // 默认最大AI坦克数量 (会被config覆盖)
              m_defaultAITankType("ai_default"),          // 默认AI类型 (会被config覆盖)
              m_defaultAITankSpeed(30.f)   ,               // 默认AI速度 (会被config覆盖)
              m_defaultAIBaseHealth(80), // Game 成员变量
              m_defaultAIBaseAttack(15),
              m_defaultAIFrameWidth(50),
              m_defaultAIFrameHeight(50)
{
    std::cout << "Game constructor called." << std::endl;
}

// Game 类的析构函数
Game::~Game() {
    std::cout << "Game destructor called." << std::endl;
}

// 从 JSON 配置文件加载纹理路径并创建纹理对象
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
                std::string path = pathNode.get<std::string>();
                if (!loadTextureFromJson(key, path)) {
                    std::cerr << "Warning: Failed to load prop texture '" << key << "' from path: " << path << std::endl;
                } else {
                    std::cout << "Loaded prop texture: " << key << std::endl;
                }
            }
        }

        // --- 加载地图瓦片纹理 ---
        if (m_configJson.contains("textures") && m_configJson["textures"].contains("map_tiles")) {
            for (auto& [key, pathNode] : m_configJson["textures"]["map_tiles"].items()) {
                std::string path = pathNode.get<std::string>();
                if (!loadTextureFromJson("map_" + key, path)) { // 给地图瓦片键名加上 "map_" 前缀
                    std::cerr << "Warning: Failed to load map_tile texture '" << key << "' from path: " << path << std::endl;
                } else {
                    std::cout << "Loaded map_tile texture: map_" << key << std::endl;
                }
            }
        }

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

                    if (pathsNode.is_array()) {
                        std::vector<sf::Texture> frameTextures;
                        for (const auto& pathNodeFrame : pathsNode) {
                            sf::Texture tempTexture;
                            std::string path = pathNodeFrame.get<std::string>();
                            if (tempTexture.loadFromFile(path)) {
                                frameTextures.push_back(tempTexture);
                            } else {
                                std::cerr << "Warning: Failed to load tank texture frame for type '" << tankType << "', dir '" << dirStr << "' from path: " << path << std::endl;
                            }
                        }
                        if (!frameTextures.empty()) {
                            m_tankTextureCache[tankType][dirEnum] = frameTextures;
                            std::cout << "  Loaded " << frameTextures.size() << " frames for " << tankType << " - " << dirStr << std::endl;
                        }
                    } else { // 单帧纹理
                        sf::Texture singleTexture;
                        std::string path = pathsNode.get<std::string>();
                        if (singleTexture.loadFromFile(path)) {
                            m_tankTextureCache[tankType][dirEnum] = {singleTexture}; // 存为只包含一帧的vector
                            std::cout << "  Loaded 1 frame (as single texture) for " << tankType << " - " << dirStr << std::endl;
                        } else {
                            std::cerr << "Warning: Failed to load single tank texture for type '" << tankType << "', dir '" << dirStr << "' from path: " << path << std::endl;
                        }
                    }
                }
            }
        }

        // --- 加载子弹纹理 ---
        if (m_configJson.contains("textures") && m_configJson["textures"].contains("bullets")) {
            for (auto& [dirStr, pathNode] : m_configJson["textures"]["bullets"].items()) {
                std::string path = pathNode.get<std::string>();
                std::string bulletKey = "bullet_" + dirStr; // 例如: "bullet_up"
                if (!loadTextureFromJson(bulletKey, path)) {
                    std::cerr << "Warning: Failed to load bullet texture for direction '" << dirStr << "' from path: " << path << std::endl;
                } else {
                    std::cout << "Loaded bullet texture: " << bulletKey << std::endl;
                }
            }
        }

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

// 根据键和路径加载单个纹理到 m_textureCache
bool Game::loadTextureFromJson(const std::string& key, const std::string& path) {
    sf::Texture texture;
    if (!texture.loadFromFile(path)) {
        // 错误信息已在调用处打印
        return false;
    }
    m_textureCache[key] = texture;
    return true;
}

// 从 m_textureCache 中获取单个纹理
const sf::Texture& Game::getTexture(const std::string& key) const {
    auto it = m_textureCache.find(key);
    if (it != m_textureCache.end()) {
        return it->second;
    }
    // 未找到纹理，返回一个静态的空纹理对象，并打印错误信息
    static sf::Texture emptyTexture; // 静态空纹理，避免每次都创建
    std::cerr << "Game::getTexture() Error: Texture with key '" << key << "' not found in cache." << std::endl;
    return emptyTexture;
}

// 从 m_tankTextureCache 中获取特定坦克类型和方向的动画帧纹理列表
const std::vector<sf::Texture>& Game::getTankTextures(const std::string& tankType, Direction dir) const {
    auto typeIt = m_tankTextureCache.find(tankType);
    if (typeIt != m_tankTextureCache.end()) {
        auto dirIt = typeIt->second.find(dir);
        if (dirIt != typeIt->second.end()) {
            return dirIt->second;
        }
    }
    static std::vector<sf::Texture> emptyTankTextures; // 静态空列表
    std::cerr << "Game::getTankTextures() Error: Tank textures not found for type '" << tankType << "' and direction " << static_cast<int>(dir) << "." << std::endl;
    return emptyTankTextures;
}

// 从配置加载可用道具类型
void Game::loadToolTypesFromConfig() {
    m_availableToolTypes.clear();
    if (m_configJson.contains("textures") && m_configJson["textures"].contains("props")) {
        for (auto& [key, pathNode] : m_configJson["textures"]["props"].items()) {
            m_availableToolTypes.push_back(key); // 将道具的键名（例如 "add_armor"）存起来
            std::cout << "Found available tool type from config: " << key << std::endl;
        }
    }
    if (m_availableToolTypes.empty()) {
        std::cerr << "Warning: No tool types found in config.json under textures.props. No tools will be spawned." << std::endl;
    }
}


// 初始化游戏各项设置和资源
void Game::init() {
    std::cout << "Game::init() called." << std::endl;
    window.setFramerateLimit(60);
    window.setVerticalSyncEnabled(true);

    if (!loadConfig("config.json")) { // 加载配置文件和所有纹理资源
        std::cerr << "CRITICAL ERROR: Failed to load game configuration. Exiting." << std::endl;
        window.close();
        return;
    }
    loadToolTypesFromConfig(); // 加载可用道具类型

    // 从配置中读取AI坦克生成参数
    if (m_configJson.contains("ai_settings")) {
        m_defaultAITankType = m_configJson["ai_settings"].value("default_type", m_defaultAITankType); // 使用构造函数中的值作为默认
        m_defaultAITankSpeed = m_configJson["ai_settings"].value("default_speed", m_defaultAITankSpeed);
        m_maxActiveAITanks = m_configJson["ai_settings"].value("max_active", m_maxActiveAITanks);
        float spawnIntervalSeconds = m_configJson["ai_settings"].value("spawn_interval_seconds", m_aiTankSpawnInterval.asSeconds());
        m_aiTankSpawnInterval = sf::seconds(spawnIntervalSeconds);

        std::cout << "AI Settings loaded from config: Type=" << m_defaultAITankType
                  << ", Speed=" << m_defaultAITankSpeed
                  << ", MaxActive=" << m_maxActiveAITanks
                  << ", SpawnInterval=" << spawnIntervalSeconds << "s" << std::endl;
    } else {
        std::cout << "AI Settings not found in config.json, using hardcoded defaults (from constructor)." << std::endl;
    }

    if(!m_map.load(*this)){ // 初始化并加载地图
        std::cerr << "CRITICAL ERROR: Failed to load map in Game::init()" << std::endl;
        window.close();
        return;
    }
    std::cout << "Map loaded successfully." << std::endl;

    // 加载玩家坦克
    auto player = std::make_unique<PlayerTank>(sf::Vector2f (300.f, 375.f), Direction::UP, *this);
    m_playerTankPtr = player.get();
    m_all_tanks.push_back(std::move(player));
    std::cout << "Player tank (type 'player') loaded successfully." << std::endl;

    // 初始AI坦克 (可以选择在这里创建一些，或者完全依赖定时生成器)
    // spawnNewAITank(); // 例如，在游戏开始时就生成一个AI
    // spawnNewAITank();


    // 为已存在的AI坦克设置目标 (例如初始AI)
    std::vector<AITank*> ai_raw_pointers;
    for (const auto& tank_ptr : m_all_tanks) { // 遍历所有已存在的坦克
        if (AITank* ai = dynamic_cast<AITank*>(tank_ptr.get())) {
            ai_raw_pointers.push_back(ai);
        }
    }
    sf::Vector2i baseTile = m_map.getBaseTileCoordinate();
    if (baseTile.x != -1 && baseTile.y != -1) {
        for (AITank* pAiTank : ai_raw_pointers) {
            if (pAiTank) {
                pAiTank->setStrategicTargetTile(baseTile);
                std::cout << "Initial AI Tank (type '" << pAiTank->getTankType() << "') target set to base." << std::endl;
            }
        }
    } else {
        std::cerr << "Game: Failed to get base tile coordinate for AI setup. Initial AI will idle or move randomly." << std::endl;
    }

    m_toolSpawnTimer = sf::Time::Zero; // 重置道具生成计时器
    m_aiTankSpawnTimer = sf::Time::Zero; // 重置AI坦克生成计时器
    initializeBulletPool();
}

// 将新创建的子弹添加到游戏世界中
void Game::addBullet(std::unique_ptr<Bullet> bullet) {
    if(bullet){ // 确保指针有效
        m_bullets.push_back(std::move(bullet));
    }
}

// 游戏主循环
void Game::run() {
    while (window.isOpen()) { // 当窗口打开时持续循环
        sf::Time deltaTime = clock.restart(); // 获取自上次调用以来经过的时间

        Handling_events(deltaTime); // 处理事件和输入
        update(deltaTime);          // 更新游戏逻辑
        render();                   // 渲染游戏画面
    }
}

// 处理窗口事件和用户输入
void Game::Handling_events(sf::Time deltaTime) {
    sf::Event event{}; // 用于存储事件
    while (window.pollEvent(event)) { // 轮询所有未处理的事件
        if (event.type == sf::Event::Closed) { // 如果是关闭窗口事件
            window.close(); // 关闭窗口
        }
        if (event.type == sf::Event::KeyPressed) { // 如果是键盘按下事件
            if (event.key.code == sf::Keyboard::Space) { // 如果按下的是空格键
                    if (!m_playerTankPtr->isDestroyed() && m_playerTankPtr->canShoot()) { // 玩家坦克存在、未被摧毁且可以射击
                        m_playerTankPtr->shoot(*this);
                    }
            }
        }
        // 处理玩家坦克的持续按键移动
        if (m_playerTankPtr && !m_playerTankPtr->isDestroyed()) {
            float distance = m_playerTankPtr->getSpeed() * deltaTime.asSeconds();
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
                sf::Vector2f targetLocation = m_playerTankPtr->get_position() + sf::Vector2f(-distance, 0.f);
                m_playerTankPtr->setDirection(Direction::LEFT, *this);
                m_playerTankPtr->move(targetLocation, m_map);
            } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
                sf::Vector2f targetLocation = m_playerTankPtr->get_position() + sf::Vector2f(distance, 0.f);
                m_playerTankPtr->setDirection(Direction::RIGHT, *this);
                m_playerTankPtr->move(targetLocation, m_map);
            } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
                sf::Vector2f targetLocation = m_playerTankPtr->get_position() + sf::Vector2f(0.f, -distance);
                m_playerTankPtr->setDirection(Direction::UP, *this);
                m_playerTankPtr->move(targetLocation, m_map);
            } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
                sf::Vector2f targetLocation = m_playerTankPtr->get_position() + sf::Vector2f(0.f, distance);
                m_playerTankPtr->setDirection(Direction::DOWN, *this);
                m_playerTankPtr->move(targetLocation, m_map);
            }
        }
    }
}


// 游戏结束时的清理工作 (如果需要)
void Game::end() {
    std::cout << "Game::end() called." << std::endl;
    // 例如：保存游戏状态、释放大型资源等
}


// --- 道具相关方法 ---
void Game::spawnRandomTool() {
    if (m_availableToolTypes.empty()) {
        // std::cout << "No available tool types to spawn." << std::endl;
        return;
    }
    if (m_tools.size() >= 5) { // 限制屏幕上最多同时存在的道具数量，例如5个
        // std::cout << "Maximum number of tools reached on map. Skipping spawn." << std::endl;
        return;
    }

    // 1. 随机选择一种道具类型
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distribType(0, m_availableToolTypes.size() - 1);
    std::string randomToolKey = m_availableToolTypes[distribType(gen)];

    // 2. 随机选择一个可行的地图位置
    sf::Vector2f spawnPosition;
    bool positionFound = false;
    int maxAttempts = 100; // 尝试找位置的最大次数
    int tileW = m_map.getTileWidth();
    int tileH = m_map.getTileHeight();

    if (tileW <= 0 || tileH <= 0) {
        std::cerr << "spawnRandomTool Error: Invalid tile dimensions from map." << std::endl;
        return;
    }

    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        // 在地图边界内随机选择瓦片坐标 (避开最外层边界墙)
        std::uniform_int_distribution<> distribX(1, m_map.getMapWidth() - 2);
        std::uniform_int_distribution<> distribY(1, m_map.getMapHeight() - 2);
        int tileX = distribX(gen);
        int tileY = distribY(gen);

        if (m_map.isTileWalkable(tileX, tileY)) {
            // 确保这个位置上没有其他道具 (简单检查)
            bool toolAlreadyThere = false;
            sf::FloatRect newToolProspectiveBounds(
                    static_cast<float>(tileX * tileW), static_cast<float>(tileY * tileH),
                    static_cast<float>(tileW), static_cast<float>(tileH) // 假设道具大致占据一个瓦片
            );
            for (const auto& existingTool : m_tools) {
                if (existingTool && existingTool->isActive() && existingTool->getBound().intersects(newToolProspectiveBounds)) {
                    toolAlreadyThere = true;
                    break;
                }
            }
            if (!toolAlreadyThere) {
                // 将位置设置在瓦片中心
                spawnPosition = sf::Vector2f(
                        static_cast<float>(tileX * tileW) + tileW / 2.0f,
                        static_cast<float>(tileY * tileH) + tileH / 2.0f
                );
                positionFound = true;
                break;
            }
        }
    }

    if (!positionFound) {
        std::cout << "spawnRandomTool: Could not find a suitable walkable tile to spawn a tool after " << maxAttempts << " attempts." << std::endl;
        return;
    }

    // 3. 创建道具对象
    const sf::Texture& toolTexture = getTexture(randomToolKey);
    if (toolTexture.getSize().x == 0 || toolTexture.getSize().y == 0) {
        std::cerr << "spawnRandomTool Error: Failed to get texture for tool key '" << randomToolKey << "'" << std::endl;
        return;
    }

    std::unique_ptr<Tools> newTool = nullptr;
    // 使用 if-else if 根据 randomToolKey 创建对应的道具实例
    if (randomToolKey == "add_armor") {
        newTool = std::make_unique<AddArmor>(spawnPosition, toolTexture);
    } else if (randomToolKey == "add_attack") {
        newTool = std::make_unique<AddAttack>(spawnPosition, toolTexture);
    } else if (randomToolKey == "add_attack_speed") {
        newTool = std::make_unique<AddAttackSpeed>(spawnPosition, toolTexture);
    } else if (randomToolKey == "add_speed") {
        newTool = std::make_unique<AddSpeed>(spawnPosition, toolTexture);
    } else if (randomToolKey == "grenade") {
        newTool = std::make_unique<GrenadeTool>(spawnPosition, toolTexture);
    } else if (randomToolKey == "slow_down_ai") {
        newTool = std::make_unique<SlowDownAI>(spawnPosition, toolTexture);
    } else {
        std::cerr << "spawnRandomTool Error: Unknown tool key '" << randomToolKey << "'" << std::endl;
        return;
    }

    if (newTool) {
        m_tools.push_back(std::move(newTool));
        std::cout << "Spawned tool '" << randomToolKey << "' at (" << spawnPosition.x << ", " << spawnPosition.y << ")" << std::endl;
    }
}

void Game::resolveTankToolCollision(Tank* tank, Tools* tool) {
    if (!tank || tank->isDestroyed() || !tool || !tool->isActive()) {
        return;
    }
    // 假设只有玩家能拾取道具，可以根据需要修改
    PlayerTank* player = dynamic_cast<PlayerTank*>(tank);
    if (player) {
        // 可以获取道具类型名作更详细的日志，如果Tools基类有getName()之类的方法
        std::cout << "PlayerTank collided with an active tool. Applying effect." << std::endl;
        tool->applyEffect(*player, *this); // 调用道具的 applyEffect
        // 道具的 setActive(false) 应该在其 applyEffect 方法中调用
    }
}

void Game::updateTools(sf::Time dt) {
    // 1. 更新道具生成计时器
    m_toolSpawnTimer += dt;
    if (m_toolSpawnTimer >= m_toolSpawnInterval) {
        spawnRandomTool();
        m_toolSpawnTimer = sf::Time::Zero; // 重置计时器
    }

    // 2. 更新所有活动道具 (如果它们有自己的 update 逻辑)
    for (auto& tool : m_tools) {
        if (tool && tool->isActive()) {
            tool->updata(dt); // 确保 Tools 类及其派生类有 update 方法 (之前可能是 updata)
        }
    }

    // 3. 处理坦克与道具的碰撞
    for (auto& tankPtr : m_all_tanks) {
        if (tankPtr && !tankPtr->isDestroyed()) {
            for (auto& toolPtr : m_tools) {
                if (toolPtr && toolPtr->isActive()) { // 只与活动的道具碰撞
                    if (tankPtr->getBounds().intersects(toolPtr->getBound())) {
                        resolveTankToolCollision(tankPtr.get(), toolPtr.get());
                        // 碰撞并应用效果后，道具通常会失效 (在其 applyEffect 中设置)
                        // 避免一帧内被多个坦克拾取或一个坦克重复拾取
                        if (!toolPtr->isActive()) break; // 如果道具已失效，跳出内层循环
                    }
                }
            }
        }
    }

    // 4. 清理不再活动的道具
    m_tools.erase(std::remove_if(m_tools.begin(), m_tools.end(),
                                 [](const std::unique_ptr<Tools>& t) {
                                     return !t || !t->isActive(); // 移除空指针或不活动的道具
                                 }),
                  m_tools.end());
}

// --- AI坦克生成方法 ---
void Game::spawnNewAITank() {
    // 1. 统计当前活跃AI坦克的数量
    int currentAICount = 0;
    for (const auto& tank_ptr : m_all_tanks) {
        if (tank_ptr) {
            AITank* ai = dynamic_cast<AITank*>(tank_ptr.get());
            if (ai && !ai->isDestroyed()) {
                currentAICount++;
            }
        }
    }

    // 2. 如果达到最大活跃AI坦克数，则不生成新的
    if (currentAICount >= m_maxActiveAITanks) {
        return;
    }

    // 3. 确定新AI坦克的出生位置
    sf::Vector2f spawnPosition;
    bool positionFound = false;
    int maxAttempts = 50;
    int tileW = m_map.getTileWidth();
    int tileH = m_map.getTileHeight();

    if (tileW <= 0 || tileH <= 0) {
        std::cerr << "spawnNewAITank Error: Invalid tile dimensions from map. Cannot determine spawn position." << std::endl;
        return;
    }

    int minYTile = 1;
    int maxYTile = m_map.getMapHeight() / 2;
    if (maxYTile <= minYTile) maxYTile = minYTile + 1;
    if (maxYTile >= m_map.getMapHeight() -1 ) maxYTile = m_map.getMapHeight() -2;

    std::random_device rd;
    std::mt19937 gen(rd());

    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        std::uniform_int_distribution<> distribX(1, m_map.getMapWidth() - 2);
        std::uniform_int_distribution<> distribY(minYTile, maxYTile);
        int tileX = distribX(gen);
        int tileY = distribY(gen);

        if (m_map.isTileWalkable(tileX, tileY)) {
            sf::Vector2f prospectiveCenter(
                    static_cast<float>(tileX * tileW) + tileW / 2.0f,
                    static_cast<float>(tileY * tileH) + tileH / 2.0f
            );
            bool tankAlreadyThere = false;
            float minDistanceSq = static_cast<float>((tileW * 0.8f) * (tileW * 0.8f));

            for (const auto& existingTank : m_all_tanks) {
                if (existingTank && !existingTank->isDestroyed()) {
                    sf::Vector2f diff = existingTank->get_position() - prospectiveCenter;
                    float distSq = diff.x * diff.x + diff.y * diff.y;
                    if (distSq < minDistanceSq) {
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
        std::cout << "spawnNewAITank: Could not find a suitable walkable tile in the upper map area after " << maxAttempts << " attempts." << std::endl;
        return;
    }

    // 4. 随机选择一个初始方向
    std::uniform_int_distribution<> distribDir(0, 3);
    Direction startDir = static_cast<Direction>(distribDir(gen));

    // 5. 创建新的AI坦克实例，使用 Game 类中硬编码的默认基础属性
    // AITank 的构造函数会负责在这些基础上进行随机化

    auto newAITank = std::make_unique<AITank>(
            spawnPosition,
            startDir,
            m_defaultAITankType,    // Game 成员变量
            *this,
            m_defaultAITankSpeed,   // Game 成员变量
            m_defaultAIBaseHealth,  // Game 成员变量
            m_defaultAIBaseAttack,   // Game 成员变量
            m_defaultAIFrameWidth,   // Game 成员变量
            m_defaultAIFrameHeight  // Game 成员变量
            // 如果 AITank 构造函数需要 frameWidth/Height，你需要确保 Game 类也有对应的
            // m_defaultAIFrameWidth 和 m_defaultAIFrameHeight 成员并在这里传递
            // 例如: m_defaultAIFrameWidth, m_defaultAIFrameHeight
    );

    AITank* aiPtr = newAITank.get();
    m_all_tanks.push_back(std::move(newAITank));

    // 6. 为新生成的AI坦克设置战略目标
    if (aiPtr) {
        sf::Vector2i baseTile = m_map.getBaseTileCoordinate();
        if (baseTile.x != -1 && baseTile.y != -1) {
            aiPtr->setStrategicTargetTile(baseTile);
            // AITank的构造函数应该会打印随机化后的实际属性
            // std::cout << "  New AI Tank (actual stats after randomization - Type: " << aiPtr->getTankType()
            //           << ") target set to base (" << baseTile.x << ", " << baseTile.y << ")" << std::endl;
        } else {
            std::cerr << "  Could not set target for new AI tank: Base tile not found." << std::endl;
        }
    } else {
        std::cerr << "Failed to create new AITank instance." << std::endl;
    }
}

void Game::updateAITankSpawning(sf::Time dt) {
    m_aiTankSpawnTimer += dt;
    if (m_aiTankSpawnTimer >= m_aiTankSpawnInterval) {
        spawnNewAITank();
        m_aiTankSpawnTimer = sf::Time::Zero; // 重置计时器
    }
}


// 更新游戏世界中所有对象的状态和逻辑
void Game::update(sf::Time dt) {
    // 1. 更新所有坦克的基础状态 (动画、通用计时器等)
    for(auto& tankPtr : m_all_tanks) {
        if(tankPtr && !tankPtr->isDestroyed()) {
            tankPtr->update(dt, *this); // Tank::update (及其派生类覆盖版本) 现在需要 Game& game 参数
        }
    }

    // 2. 更新AI坦克的特定逻辑 (移动决策、格子间移动、自动射击)
    for (auto& tankPtr : m_all_tanks) {
        if (tankPtr && !tankPtr->isDestroyed()) {
            AITank* aiTankPtr = dynamic_cast<AITank*>(tankPtr.get());
            if (aiTankPtr) { // 如果是AI坦克
                if (!aiTankPtr->isMoving()) { // 如果AI当前没有在进行格子间移动
                    aiTankPtr->decideNextAction(m_map, m_playerTankPtr); // AI决策下一步行动
                }
                aiTankPtr->updateMovementBetweenTiles(dt, m_map); // 更新AI的格子间平滑移动

                if (aiTankPtr->canShootAI()) { // 检查AI是否可以射击 (基于其特定冷却)
                    aiTankPtr->shoot(*this); // AI射击
                    aiTankPtr->resetShootTimerAI(); // 重置AI的射击计时器
                    }
                }
            }
        }

    // 3. 处理坦克间的碰撞
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

    // 4. 更新所有子弹的状态 (移动) - 只更新活跃的
    for (auto& bullet_ptr : m_bulletPool) { // 遍历整个池
        if (bullet_ptr && bullet_ptr->isAlive()) { // 只更新活跃的
            bullet_ptr->update(dt);
        }
    }

// 5. 处理碰撞逻辑
//    a. 子弹与坦克的碰撞 - 只处理活跃的
    for (auto& bullet_ptr : m_bulletPool) {
        if (bullet_ptr && bullet_ptr->isAlive()) { // 只检查活跃的子弹
            for (auto& tankPtr : m_all_tanks) {
                // ... (碰撞逻辑不变, 但确保 tankPtr 也有效且未摧毁) ...
                if (tankPtr && !tankPtr->isDestroyed()) {
                    if (bullet_ptr->getBounds().intersects(tankPtr->getBounds())) {
                        // ... (自伤避免逻辑) ...
                        // if (!potential_self_hit) { // 假设这个逻辑存在
                        tankPtr->takeDamage(bullet_ptr->getDamage());
                        bullet_ptr->setIsAlive(false); // 子弹失效，返回池中（逻辑上）
                        break;
                        // }
                    }
                }
            }
        }
    }
//    b. 子弹与地图的碰撞 - 只处理活跃的
    for (auto& bullet_ptr : m_bulletPool) {
        if (bullet_ptr && bullet_ptr->isAlive()) { // 只检查活跃的子弹
            // ... (与地图的碰撞逻辑不变，击中后 bullet_ptr->setIsAlive(false);)
            sf::Vector2f bulletPos = bullet_ptr->getPosition();
            int tileX = static_cast<int>(bulletPos.x / m_map.getTileWidth());
            int tileY = static_cast<int>(bulletPos.y / m_map.getTileHeight());

            if (tileX >= 0 && tileX < m_map.getMapWidth() && tileY >= 0 && tileY < m_map.getMapHeight()) {
                int tileTypeHit = m_map.getTileType(tileX, tileY);
                if (tileTypeHit == 1) {
                    m_map.damageTile(tileX, tileY,1,*this);
                    bullet_ptr->setIsAlive(false);}
                else if(tileTypeHit == 3){
                    m_map.damageTile(tileX, tileY,bullet_ptr->getDamage(),*this);
                    bullet_ptr->setIsAlive(false);
                    if(m_map.isBaseDestroyed()){
                        std::cout << "Base destroyed!" << std::endl;
                        state = GameState::GameOver;
                    }
                }else if(tileTypeHit == 2) {
                    bullet_ptr->setIsAlive(false);
                }
            } else {
                bullet_ptr->setIsAlive(false); // 飞出边界
            }
        }
    }


    // 更新道具逻辑
    updateTools(dt);

    // 更新AI坦克生成逻辑
    updateAITankSpawning(dt);

    // 6. 清理不再存活的实体

    //    a. 清理被摧毁的坦克
    m_all_tanks.erase(std::remove_if(m_all_tanks.begin(), m_all_tanks.end(),
                                     [&](const std::unique_ptr<Tank>& tank_to_check) {
                                         bool should_remove = tank_to_check && tank_to_check->isDestroyed();
                                         if (should_remove) {
                                             if (m_playerTankPtr == tank_to_check.get()) {
                                                 // m_playerTankPtr 会在下面被置空
                                             }
                                         }
                                         return should_remove;
                                     }),
                      m_all_tanks.end());

    // 7. 在清理完坦克后，检查并更新 m_playerTankPtr
    if (m_playerTankPtr) { // 如果之前玩家坦克指针有效
        bool player_found_in_list = false;
        for (const auto& tank_ptr : m_all_tanks) { // 再次遍历坦克列表
            if (tank_ptr.get() == m_playerTankPtr) { // 检查指针是否仍然指向列表中的某个坦克
                player_found_in_list = true;
                break;
            }
        }
        if (!player_found_in_list) { // 如果在列表中没找到，说明玩家坦克已被移除
            m_playerTankPtr = nullptr; // 将指针置空
            std::cout << "Player tank has been removed from game. m_playerTankPtr is now nullptr." << std::endl;
            // 在这里可以添加游戏结束的逻辑，例如切换到 GameOver 状态
            // state = GameState::GameOver;
            // window.close(); // 或者直接关闭游戏
        }
    } else { // 如果 m_playerTankPtr 本来就是 nullptr
        if (state == GameState::Playing1P) { // 如果是单人游戏且玩家指针为空 (意味着玩家坦克被摧毁且已清理)
            std::cout << "Player is null, game over!" << std::endl;
            // state = GameState::GameOver;
            // window.close(); // 简单处理：直接关闭窗口
        }
    }
    // 8.检查基地是否被摧毁
    if (m_map.isBaseDestroyed() && state != GameState::GameOver) { // 检查基地是否已被摧毁
        std::cout << "Game Over! Base was destroyed." << std::endl;
        // state = GameState::GameOver; // 切换到游戏结束状态
        // window.close(); // 或者暂时直接关闭窗口
        // 在这里通常会停止游戏更新或显示游戏结束画面
    }

    // 如果游戏已结束，你可能想跳过后续的更新
    if (state == GameState::GameOver) {
        // 这里可以处理游戏结束画面的更新或重新开始/退出的输入
        return;
    }

    // 9. 其他游戏逻辑 (例如检查游戏胜利/失败条件)

    // ...
}

// 将游戏世界渲染到窗口上
void Game::render() {
    window.clear(sf::Color(100, 100, 100)); // 清屏，使用深灰色背景

    // 绘制地图
    m_map.draw(window, *this);

    // 绘制所有坦克
    for (const auto &tank: m_all_tanks) {
        if (tank && !tank->isDestroyed()) { // 只绘制存活的坦克
            tank->draw(window);
        }
    }

    // 绘制所有子弹
    for (const auto &bullet_ptr: m_bulletPool) { // 遍历整个池
        if (bullet_ptr && bullet_ptr->isAlive()) { // 只绘制活跃的
            bullet_ptr->draw(window);
        }
    }

    // 绘制所有活动道具
    for (const auto &tool : m_tools) {
        if (tool && tool->isActive()) { // 只绘制活动的道具
            tool->draw(window);
        }
    }

    // (可选) 绘制UI元素 (得分、生命值等)
    // ...

    window.display(); // 显示绘制的内容
}

// 解决坦克之间的碰撞 (简单推开逻辑)
void Game::resolveTankCollision(Tank* tank1, Tank* tank2) {
    if (!tank1 || !tank2 || tank1->isDestroyed() || tank2->isDestroyed()) return;

    sf::FloatRect bounds1 = tank1->getBounds();
    sf::FloatRect bounds2 = tank2->getBounds();
    sf::FloatRect intersection;

    if (bounds1.intersects(bounds2, intersection)) {
        sf::Vector2f pos1 = tank1->get_position();
        sf::Vector2f pos2 = tank2->get_position();

        sf::Vector2f pushDirection = pos1 - pos2;
        if (pushDirection.x == 0 && pushDirection.y == 0) {
            pushDirection = sf::Vector2f(0.f, -1.f); // 默认推开方向
        }

        float length = std::sqrt(pushDirection.x * pushDirection.x + pushDirection.y * pushDirection.y);
        if (length != 0) {
            pushDirection /= length;
        }

        float pushMagnitude = (std::min(intersection.width, intersection.height) / 2.0f) + 0.5f;
        sf::Vector2f moveOffset = pushDirection * pushMagnitude;

        tank1->move(pos1 + moveOffset, m_map);
        tank2->move(pos2 - moveOffset, m_map);

        // AI坦克碰撞后可能需要重新规划路径
        AITank* ai1 = dynamic_cast<AITank*>(tank1);
        AITank* ai2 = dynamic_cast<AITank*>(tank2);
        if (ai1 && ai1->isMoving()) { /* ai1->forceReplanPath(); */ } // 假设有这样的方法
        if (ai2 && ai2->isMoving()) { /* ai2->forceReplanPath(); */ }
    }
}

void Game::initializeBulletPool() {
    m_bulletPool.clear(); // 如果可能被多次调用，先清空
    m_bulletPool.reserve(INITIAL_BULLET_POOL_SIZE); // 预留空间，提高效率

    // 为了创建 Bullet 对象，我们需要一个有效的纹理。
    // 我们假设 "bullet_up" (或任何一个子弹纹理键) 此时必定已经加载。
    // 如果没有，这将是一个严重问题，应该在 loadConfig 中处理。
    const sf::Texture& defaultBulletTexture = getTexture("bullet_up"); // 获取一个默认的子弹纹理

    if (defaultBulletTexture.getSize().x == 0 || defaultBulletTexture.getSize().y == 0) {
        std::cerr << "CRITICAL ERROR: Default bullet texture ('bullet_up') is not loaded or invalid. Cannot pre-allocate bullet pool." << std::endl;
        // 在这种情况下，getAvailableBullet() 可能会在尝试创建新子弹时失败。
        // 或者，你可以选择在这里抛出异常或关闭游戏。
        return;
    }

    std::cout << "Pre-allocating bullet pool with " << INITIAL_BULLET_POOL_SIZE << " bullets..." << std::endl;
    for (size_t i = 0; i < INITIAL_BULLET_POOL_SIZE; ++i) {
        // 创建子弹对象，并使用占位符/默认值进行初始化。
        // 重要的是将 isAlive 设置为 false。
        // 实际的参数（位置、方向、伤害等）将在从池中取出并调用 reset() 时设置。
        auto bullet = std::make_unique<Bullet>(
                defaultBulletTexture,        // 初始纹理
                sf::Vector2f(0.f, 0.f),      // 初始位置 (占位)
                Direction::UP,               // 初始方向 (占位)
                sf::Vector2f(0.f, -1.f),     // 飞行向量 (占位)
                0,                           // 伤害 (占位)
                0.f,                         // 速度 (占位)
                0                            // 类型 (占位)
        );
        bullet->setIsAlive(false); // **非常重要：新创建的池对象必须是不活跃的**
        m_bulletPool.push_back(std::move(bullet));
    }
    std::cout << "Bullet pool pre-allocated. Size: " << m_bulletPool.size() << std::endl;
}

Bullet* Game::getAvailableBullet() {
    // 1. 尝试找到一个不活跃的子弹并复用
    for (const auto& bullet_ptr : m_bulletPool) {
        if (bullet_ptr && !bullet_ptr->isAlive()) {
            // bullet_ptr->setIsAlive(true); // 调用者在 reset 后会激活它
            // std::cout << "Reusing bullet from pool." << std::endl;
            return bullet_ptr.get();
        }
    }

    // 2. 如果池中所有子弹都在使用中，则动态创建一个新的 (如果允许扩展)
    // 你可以设置一个最大池大小上限 (例如 MAX_BULLET_POOL_SIZE) 来防止无限创建
    // if (m_bulletPool.size() >= MAX_BULLET_POOL_SIZE) {
    //    std::cerr << "Max bullet pool size reached. Cannot provide more bullets." << std::endl;
    //    return nullptr;
    // }

    std::cout << "Bullet pool fully active, creating a new bullet instance." << std::endl;
    const sf::Texture& defaultBulletTexture = getTexture("bullet_up"); // 再次确保纹理有效
    if (defaultBulletTexture.getSize().x == 0) {
        std::cerr << "CRITICAL ERROR: Default bullet texture for new bullet is invalid in getAvailableBullet!" << std::endl;
        return nullptr;
    }

    auto new_bullet = std::make_unique<Bullet>(
            defaultBulletTexture, sf::Vector2f(0.f, 0.f), Direction::UP, sf::Vector2f(0.f, -1.f), 0, 0.f, 0
    );
    // new_bullet->setIsAlive(true); // 调用者在 reset 时会激活它，这里创建后应该是“准备好被reset”的状态
    new_bullet->setIsAlive(false); // 先设为不活跃，让调用者reset并激活
    Bullet* raw_ptr = new_bullet.get();
    m_bulletPool.push_back(std::move(new_bullet));
    std::cout << "New bullet added to pool. Pool size: " << m_bulletPool.size() << std::endl;
    return raw_ptr;
}

