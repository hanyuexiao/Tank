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
              m_defaultAITankSpeed(30.f)                  // 默认AI速度 (会被config覆盖)
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
        if(event.type == sf::Event::Closed){ // 如果是关闭窗口事件
            window.close(); // 关闭窗口
        }
        if( event.type == sf::Event::KeyPressed){ // 如果是键盘按下事件
            if(event.key.code == sf::Keyboard::Space){ // 如果按下的是空格键
                std::cout << "Spacebar pressed." << std::endl; // 调试日志
                if (m_playerTankPtr) { // 确保玩家坦克指针有效
                    // 打印射击条件检查的详细日志
                    std::cout << "PlayerTank exists. IsDestroyed: " << m_playerTankPtr->isDestroyed()
                              << ", CanShoot: " << m_playerTankPtr->canShoot() << std::endl;
                    if (!m_playerTankPtr->isDestroyed() && m_playerTankPtr->canShoot()){ // 玩家坦克存在、未被摧毁且可以射击
                        std::cout << "Attempting to shoot..." << std::endl;
                        std::unique_ptr<Bullet> new_Bullet = m_playerTankPtr->shoot(*this); // 玩家坦克射击
                        if(new_Bullet){
                            addBullet(std::move(new_Bullet)); // 添加子弹到游戏世界
                            std::cout << "Player bullet CREATED and added to game." << std::endl;
                        } else {
                            std::cout << "Player bullet was NOT created by shoot() method (e.g. texture issue, or shoot returned null)." << std::endl;
                        }
                    }
                } else {
                    std::cout << "m_playerTankPtr is NULL when trying to shoot." << std::endl;
                }
            }
        }
    }

    // 处理玩家坦克的持续按键移动
    if(m_playerTankPtr && !m_playerTankPtr->isDestroyed()){
        float distance = m_playerTankPtr->getSpeed() * deltaTime.asSeconds();
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
        {
            sf::Vector2f targetLocation = m_playerTankPtr->get_position() + sf::Vector2f(-distance, 0.f);
            m_playerTankPtr->setDirection(Direction::LEFT, *this);
            m_playerTankPtr->move(targetLocation, m_map);
        }
        else if(sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
        {
            sf::Vector2f targetLocation = m_playerTankPtr->get_position() + sf::Vector2f(distance, 0.f);
            m_playerTankPtr->setDirection(Direction::RIGHT, *this);
            m_playerTankPtr->move(targetLocation, m_map);
        }
        else if(sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
        {
            sf::Vector2f targetLocation = m_playerTankPtr->get_position() + sf::Vector2f(0.f, -distance);
            m_playerTankPtr->setDirection(Direction::UP, *this);
            m_playerTankPtr->move(targetLocation, m_map);
        }
        else if(sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
        {
            sf::Vector2f targetLocation = m_playerTankPtr->get_position() + sf::Vector2f(0.f, distance);
            m_playerTankPtr->setDirection(Direction::DOWN, *this);
            m_playerTankPtr->move(targetLocation, m_map);
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
    // 统计当前AI坦克数量
    int currentAICount = 0;
    for (const auto& tank : m_all_tanks) {
        if (dynamic_cast<AITank*>(tank.get())) { // 检查是否为AITank类型
            currentAICount++;
        }
    }

    if (currentAICount >= m_maxActiveAITanks) {
        // std::cout << "Maximum number of AI tanks reached (" << currentAICount << "/" << m_maxActiveAITanks << "). Skipping AI spawn." << std::endl;
        return;
    }

    sf::Vector2f spawnPosition;
    bool positionFound = false;
    int maxAttempts = 50; // 尝试寻找出生点的最大次数
    int tileW = m_map.getTileWidth();
    int tileH = m_map.getTileHeight();

    if (tileW <= 0 || tileH <= 0) {
        std::cerr << "spawnNewAITank Error: Invalid tile dimensions from map." << std::endl;
        return;
    }

    // 定义地图上半部分的可部署区域 (例如：Y轴从第1行到地图高度的一半，X轴避开边界)
    int minYTile = 1; // 从第1行开始 (避开顶部边界墙)
    int maxYTile = m_map.getMapHeight() / 2; // 到地图高度的一半
    if (maxYTile <= minYTile) maxYTile = minYTile + 1; // 确保至少有一行可选
    if (maxYTile >= m_map.getMapHeight() -1 ) maxYTile = m_map.getMapHeight() -2; // 避免太靠近底部边界


    std::random_device rd;
    std::mt19937 gen(rd());

    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        std::uniform_int_distribution<> distribX(1, m_map.getMapWidth() - 2); // X轴避开左右边界
        std::uniform_int_distribution<> distribY(minYTile, maxYTile);        // Y轴在指定上半区域
        int tileX = distribX(gen);
        int tileY = distribY(gen);

        if (m_map.isTileWalkable(tileX, tileY)) {
            // 检查该位置是否会与其他坦克重叠 (简单检查瓦片中心是否已有坦克)
            sf::Vector2f prospectiveCenter(
                    static_cast<float>(tileX * tileW) + tileW / 2.0f,
                    static_cast<float>(tileY * tileH) + tileH / 2.0f
            );
            bool tankAlreadyThere = false;
            // 设定一个最小距离，避免坦克生成时过于靠近或重叠
            // 可以根据坦克尺寸调整这个距离，例如坦克宽度的一半或一个瓦片的宽度
            float minDistanceSq = static_cast<float>((tileW * 0.8f) * (tileW * 0.8f)); // 稍微减小判定距离，避免过于稀疏

            for (const auto& existingTank : m_all_tanks) {
                if (existingTank) {
                    sf::Vector2f diff = existingTank->get_position() - prospectiveCenter;
                    float distSq = diff.x * diff.x + diff.y * diff.y;
                    if (distSq < minDistanceSq) { // 如果距离太近
                        tankAlreadyThere = true;
                        break;
                    }
                }
            }

            if (!tankAlreadyThere) {
                spawnPosition = prospectiveCenter; // 使用瓦片中心作为生成点
                positionFound = true;
                break;
            }
        }
    }

    if (!positionFound) {
        std::cout << "spawnNewAITank: Could not find a suitable walkable tile in the upper map area after " << maxAttempts << " attempts." << std::endl;
        return;
    }

    // 随机选择一个初始方向
    std::uniform_int_distribution<> distribDir(0, 3); // 0:LEFT, 1:RIGHT, 2:UP, 3:DOWN
    Direction startDir = static_cast<Direction>(distribDir(gen));

    // 创建新的AI坦克实例
    auto newAITank = std::make_unique<AITank>(spawnPosition, startDir, m_defaultAITankType, *this, m_defaultAITankSpeed);
    AITank* aiPtr = newAITank.get(); // 获取原始指针以便设置目标

    m_all_tanks.push_back(std::move(newAITank));
    std::cout << "Spawned new AI Tank (type: " << m_defaultAITankType << ") at (" << spawnPosition.x << ", " << spawnPosition.y << ")" << std::endl;

    // 为新生成的AI坦克设置目标 (例如，玩家基地)
    if (aiPtr) {
        sf::Vector2i baseTile = m_map.getBaseTileCoordinate();
        if (baseTile.x != -1 && baseTile.y != -1) {
            aiPtr->setStrategicTargetTile(baseTile);
            std::cout << "  New AI Tank target set to base (" << baseTile.x << ", " << baseTile.y << ")" << std::endl;
        } else {
            std::cerr << "  Could not set target for new AI tank: Base tile not found." << std::endl;
        }
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
                    std::unique_ptr<Bullet> newBullet = aiTankPtr->shoot(*this); // AI射击
                    if (newBullet) {
                        addBullet(std::move(newBullet));
                        aiTankPtr->resetShootTimerAI(); // 重置AI的射击计时器
                    }
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

    // 4. 更新所有子弹的状态 (移动)
    for (auto& bullet : m_bullets) {
        if (bullet && bullet->isAlive()) {
            bullet->update(dt);
        }
    }

    // 5. 处理碰撞逻辑
    //    a. 子弹与坦克的碰撞
    for (auto& bullet : m_bullets) {
        if (bullet && bullet->isAlive()) { // 只处理存活的子弹
            for (auto& tankPtr : m_all_tanks) {
                if (tankPtr && !tankPtr->isDestroyed()) { // 只与存活的坦克碰撞
                    // 简单的自伤避免：如果子弹刚从坦克附近发射，可能暂时不检测碰撞
                    // 这需要Bullet能知道其发射者，或者基于距离和时间的启发式判断
                    // 例如： if (bullet->getOwner() == tankPtr.get() && bullet->getLifetime() < sf::milliseconds(100)) continue;
                    AITank* ai = dynamic_cast<AITank*>(tankPtr.get());
                    if(ai){
                        sf::FloatRect bounds = tankPtr->getBounds();
                        std::cout << "AI Tank (type: " << ai->getTankType() << ") bounds: " << bounds.left << ", " << bounds.top << ", " << bounds.width << ", " << bounds.height << std::endl;
                    }
                    if (bullet->getBounds().intersects(tankPtr->getBounds())) {
                        // 进一步检查，避免子弹刚发射就和发射者碰撞
                        // 假设Bullet类有一个getOwnerType()方法，并且Tank类有一个getTankType()方法
                        // 并且Tank::shoot()在创建子弹时设置了ownerType
                        bool potential_self_hit = false;
//                        if (bullet->getOwnerType() == tankPtr->getTankType()) { // 假设有getOwnerType
                            // 检查距离是否非常近，表明可能是刚发射
                            sf::Vector2f dist_vec = bullet->getPosition() - tankPtr->get_position();
                            float tank_radius_approx = (tankPtr->get_TileWight() + tankPtr->get_TileHeight()) / 4.0f; // 坦克大致半径
                            if (std::sqrt(dist_vec.x*dist_vec.x + dist_vec.y*dist_vec.y) < tank_radius_approx * 1.5f) { // 如果子弹还在坦克1.5倍半径内
                                // std::cout << "Potential self-hit avoided for " << tankPtr->getTankType() << std::endl;
                                potential_self_hit = true;
                            }
//                        }

                        if (!potential_self_hit) {
                            tankPtr->takeDamage(bullet->getDamage()); // 坦克受损
                            bullet->setIsAlive(false);                // 子弹失效
                            // std::cout << "Bullet (owner: " << bullet->getOwnerType() << ") hit Tank (type '" << tankPtr->getTankType() << "')." << std::endl;
                            break; //一颗子弹通常只击中一个目标
                        }
                    }
                }
            }
        }
    }
    //    b. 子弹与地图的碰撞
    for (auto& bullet : m_bullets) {
        if (bullet && bullet->isAlive()) {
            sf::Vector2f bulletPos = bullet->getPosition();
            int tileX = static_cast<int>(bulletPos.x / m_map.getTileWidth());
            int tileY = static_cast<int>(bulletPos.y / m_map.getTileHeight());

            if (tileX >= 0 && tileX < m_map.getMapWidth() && tileY >= 0 && tileY < m_map.getMapHeight()) { // 确保在地图边界内
                if (!m_map.isTileWalkable(tileX, tileY)) { // 如果子弹击中不可通行的瓦片
                    bullet->setIsAlive(false); // 子弹失效
                }
            } else { // 子弹飞出地图边界
                bullet->setIsAlive(false);
            }
        }
    }

    // 更新道具逻辑
    updateTools(dt);

    // 更新AI坦克生成逻辑
    updateAITankSpawning(dt);

    // 6. 清理不再存活的实体
    //    a. 清理子弹
    m_bullets.erase(std::remove_if(m_bullets.begin(), m_bullets.end(),
                                   [](const std::unique_ptr<Bullet>& b) {
                                       return !b || !b->isAlive(); // 移除空指针或不存活的子弹
                                   }),
                    m_bullets.end());

    //    b. 清理被摧毁的坦克
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

    // 8. 其他游戏逻辑 (例如检查游戏胜利/失败条件)
    // 检查基地是否被摧毁等
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
    for (const auto &bullet: m_bullets) {
        if (bullet && bullet->isAlive()) { // 只绘制存活的子弹
            bullet->draw(window);
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


