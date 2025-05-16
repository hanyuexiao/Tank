#include "game.h"       // 包含 Game 类的头文件
#include "AITank.h"     // 包含 AI坦克类 的头文件
#include "PlayerTank.h" // 包含 玩家坦克类 的头文件
#include "Bullet.h"     // 包含 子弹类 的头文件 (虽然已在 game.h 包含，但显式包含无害)
#include "Map.h"        // 包含 地图类 的头文件 (虽然已在 game.h 包含)
#include <iostream>     // 用于标准输入输出 (例如 std::cout, std::cerr)
#include <fstream>      // 用于文件流操作 (例如 std::ifstream)
#include <vector>       // 用于 std::vector
#include <algorithm>    // 用于 std::remove_if

// Game 类的构造函数
Game::Game(): window(sf::VideoMode(1500, 750), "Tank Battle!"), // 初始化窗口，标题为 "Tank Battle!"
              state(GameState::MainMenu), // 初始化游戏状态为主菜单
              score(0),                   // 初始化分数为 0
              life(3),                    // 初始化生命值为 3
              m_map()                     // 初始化地图对象 (默认构造)
{
    // 输出构造函数被调用的信息，用于调试
    std::cout << "Game constructor called." << std::endl;
}

// Game 类的析构函数
Game::~Game() {
    // 输出析构函数被调用的信息，用于调试
    std::cout << "Game destructor called." << std::endl;
}

// 从 JSON 配置文件加载纹理路径并创建纹理对象
bool Game::loadConfig(const std::string& configPath) {
    std::ifstream configFile(configPath); // 打开配置文件
    if (!configFile.is_open()) { // 检查文件是否成功打开
        std::cerr << "CRITICAL ERROR: Failed to open config file: " << configPath << std::endl;
        return false; // 打开失败，返回 false
    }

    try {
        configFile >> m_configJson; // 解析 JSON 文件内容到 m_configJson 对象
        std::cout << "Config file '" << configPath << "' loaded and parsed successfully." << std::endl;

        // --- 加载道具纹理 ---
        if (m_configJson.contains("textures") && m_configJson["textures"].contains("props")) {
            for (auto& [key, pathNode] : m_configJson["textures"]["props"].items()) { // 遍历道具纹理配置
                std::string path = pathNode.get<std::string>(); // 获取路径字符串
                if (!loadTextureFromJson(key, path)) { // 加载纹理
                    std::cerr << "Warning: Failed to load prop texture '" << key << "' from path: " << path << std::endl;
                } else {
                    std::cout << "Loaded prop texture: " << key << std::endl;
                }
            }
        }

        // --- 加载地图瓦片纹理 ---
        if (m_configJson.contains("textures") && m_configJson["textures"].contains("map_tiles")) {
            for (auto& [key, pathNode] : m_configJson["textures"]["map_tiles"].items()) { // 遍历地图瓦片纹理配置
                std::string path = pathNode.get<std::string>();
                // 给地图瓦片的键名加上 "map_" 前缀，以避免与道具等其他类型的纹理键名冲突
                if (!loadTextureFromJson("map_" + key, path)) {
                    std::cerr << "Warning: Failed to load map_tile texture '" << key << "' from path: " << path << std::endl;
                } else {
                    std::cout << "Loaded map_tile texture: map_" << key << std::endl;
                }
            }
        }

        // --- 加载坦克纹理 (支持多帧动画) ---
        if (m_configJson.contains("textures") && m_configJson["textures"].contains("tanks")) {
            for (auto& [tankType, directionsNode] : m_configJson["textures"]["tanks"].items()) { // 遍历不同坦克类型
                std::cout << "Loading textures for tank type: " << tankType << std::endl;
                for (auto& [dirStr, pathsNode] : directionsNode.items()) { // 遍历该坦克类型的不同方向
                    Direction dirEnum; // 将方向字符串转换为 Direction 枚举
                    if (dirStr == "up") dirEnum = Direction::UP;
                    else if (dirStr == "down") dirEnum = Direction::DOWN;
                    else if (dirStr == "left") dirEnum = Direction::LEFT;
                    else if (dirStr == "right") dirEnum = Direction::RIGHT;
                    else {
                        std::cerr << "Warning: Unknown direction string '" << dirStr << "' for tank type '" << tankType << "'" << std::endl;
                        continue; // 跳过未知方向
                    }

                    if (pathsNode.is_array()) { // 检查路径是否为一个数组 (用于动画帧)
                        std::vector<sf::Texture> frameTextures; // 存储该方向的动画帧纹理
                        for (const auto& pathNodeFrame : pathsNode) { // 遍历动画帧的路径
                            sf::Texture tempTexture;
                            std::string path = pathNodeFrame.get<std::string>();
                            if (tempTexture.loadFromFile(path)) { // 加载单帧纹理
                                frameTextures.push_back(tempTexture);
                            } else {
                                std::cerr << "Warning: Failed to load tank texture frame for type '" << tankType << "', dir '" << dirStr << "' from path: " << path << std::endl;
                            }
                        }
                        if (!frameTextures.empty()) {
                            // 将加载的动画帧纹理存入坦克纹理缓存
                            m_tankTextureCache[tankType][dirEnum] = frameTextures;
                            std::cout << "  Loaded " << frameTextures.size() << " frames for " << tankType << " - " << dirStr << std::endl;
                        }
                    } else { // 如果不是数组，则假定为单个纹理路径 (虽然坦克通常有动画)
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
            for (auto& [dirStr, pathNode] : m_configJson["textures"]["bullets"].items()) { // 遍历子弹纹理配置 (按方向)
                std::string path = pathNode.get<std::string>();
                std::string bulletKey = "bullet_" + dirStr; // 例如: "bullet_up", "bullet_left"
                if (!loadTextureFromJson(bulletKey, path)) {
                    std::cerr << "Warning: Failed to load bullet texture for direction '" << dirStr << "' from path: " << path << std::endl;
                } else {
                    std::cout << "Loaded bullet texture: " << bulletKey << std::endl;
                }
            }
        }

    } catch (nlohmann::json::parse_error& e) { // 捕获 JSON 解析异常
        std::cerr << "CRITICAL ERROR: JSON parsing failed: " << e.what() << std::endl;
        return false; // 解析失败，返回 false
    } catch (nlohmann::json::type_error& e) { // 捕获 JSON 类型错误 (例如，期望字符串但得到数字)
        std::cerr << "CRITICAL ERROR: JSON type error: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) { // 捕获其他标准异常
        std::cerr << "CRITICAL ERROR: An unexpected error occurred during config loading: " << e.what() << std::endl;
        return false;
    }
    return true; // 配置加载和解析成功
}

// 根据键和路径加载单个纹理到 m_textureCache
bool Game::loadTextureFromJson(const std::string& key, const std::string& path) {
    sf::Texture texture;
    if (!texture.loadFromFile(path)) { // 从文件加载纹理
        // 错误信息已在调用处打印，这里可以不再重复打印，或者打印更简洁的
        // std::cerr << "Helper: Failed to load texture with key '" << key << "' from path: " << path << std::endl;
        return false; // 加载失败
    }
    m_textureCache[key] = texture; // 存入纹理缓存
    return true; // 加载成功
}

// 从 m_textureCache 中获取单个纹理
const sf::Texture& Game::getTexture(const std::string& key) const {
    auto it = m_textureCache.find(key); // 查找纹理
    if (it != m_textureCache.end()) {
        return it->second; // 找到则返回纹理引用
    }
    // 未找到纹理，返回一个静态的空纹理对象，并打印错误信息
    static sf::Texture emptyTexture; // 静态空纹理，避免每次都创建
    std::cerr << "Game::getTexture() Error: Texture with key '" << key << "' not found in cache." << std::endl;
    return emptyTexture;
}

// 从 m_tankTextureCache 中获取特定坦克类型和方向的动画帧纹理列表
const std::vector<sf::Texture>& Game::getTankTextures(const std::string& tankType, Direction dir) const {
    auto typeIt = m_tankTextureCache.find(tankType); // 查找坦克类型
    if (typeIt != m_tankTextureCache.end()) {
        auto dirIt = typeIt->second.find(dir); // 查找对应方向
        if (dirIt != typeIt->second.end()) {
            return dirIt->second; // 找到则返回纹理列表的引用
        }
    }
    // 未找到纹理，返回一个静态的空纹理列表，并打印错误信息
    static std::vector<sf::Texture> emptyTankTextures; // 静态空列表
    std::cerr << "Game::getTankTextures() Error: Tank textures not found for type '" << tankType << "' and direction " << static_cast<int>(dir) << "." << std::endl;
    return emptyTankTextures;
}


// 初始化游戏各项设置和资源
void Game::init() {
    std::cout << "Game::init() called." << std::endl;
    window.setFramerateLimit(60);       // 设置帧率上限为 60 FPS
    window.setVerticalSyncEnabled(true); // 开启垂直同步，减少画面撕裂

    // 加载配置文件和所有纹理资源
    if (!loadConfig("config.json")) { // 假设配置文件名为 config.json 且在可执行文件同目录下
        std::cerr << "CRITICAL ERROR: Failed to load game configuration. Exiting." << std::endl;
        window.close(); // 关闭窗口并退出
        return;
    }

    // 初始化并加载地图，将 Game 对象自身引用传递给 Map 类
    if(!m_map.load(*this)){ // Map::load 现在需要 Game& game 参数
        std::cerr << "CRITICAL ERROR: Failed to load map in Game::init()" << std::endl;
        window.close();
        return;
    }
    std::cout << "Map loaded successfully." << std::endl;

    // 加载玩家坦克
    // 构造 PlayerTank 时传递 Game 引用 (*this) 和坦克类型字符串 ("player")
    auto player = std::make_unique<PlayerTank>(sf::Vector2f (300.f, 375.f), Direction::UP, *this /*, 其他 PlayerTank 特定参数 */);
    m_playerTankPtr = player.get(); // 保存指向玩家坦克的原始指针
    m_all_tanks.push_back(std::move(player)); // 将玩家坦克添加到坦克列表中
    std::cout << "Player tank (type 'player') loaded successfully." << std::endl;

    // 加载AI坦克
    // 构造 AITank 时传递 Game 引用 (*this) 和坦克类型字符串 (例如 "ai_default")
    // 确保 "ai_default" 是你在 config.json 中为AI坦克定义的类型键名
    auto aiTank1 = std::make_unique<AITank>(sf::Vector2f(1230.f, 300.f), Direction::UP, "ai_default", *this, 30.f /* speed */);
    auto aiTank2 = std::make_unique<AITank>(sf::Vector2f(600.f, 300.f), Direction::UP, "ai_default", *this, 30.f);
    auto aiTank3 = std::make_unique<AITank>(sf::Vector2f(700.f, 300.f), Direction::UP, "ai_default", *this, 30.f);
    auto aiTank4 = std::make_unique<AITank>(sf::Vector2f(200.f, 300.f), Direction::UP, "ai_default", *this, 30.f);

    // (AI坦克的目标设置逻辑保持不变)
    std::vector<AITank*> ai_raw_pointers;
    if (auto ptr = dynamic_cast<AITank*>(aiTank1.get())) ai_raw_pointers.push_back(ptr);
    if (auto ptr = dynamic_cast<AITank*>(aiTank2.get())) ai_raw_pointers.push_back(ptr);
    if (auto ptr = dynamic_cast<AITank*>(aiTank3.get())) ai_raw_pointers.push_back(ptr);
    if (auto ptr = dynamic_cast<AITank*>(aiTank4.get())) ai_raw_pointers.push_back(ptr);

    sf::Vector2i baseTile = m_map.getBaseTileCoordinate();
    if (baseTile.x != -1 && baseTile.y != -1) {
        for (AITank* pAiTank : ai_raw_pointers) {
            if (pAiTank) {
                pAiTank->setStrategicTargetTile(baseTile);
                std::cout << "AI Tank (type '" << pAiTank->getTankType() << "') at (" << pAiTank->get_position().x << ", " << pAiTank->get_position().y
                          << ") target set to base." << std::endl;
            }
        }
    } else {
        std::cerr << "Game: Failed to get base tile coordinate for AI setup. All AI will idle." << std::endl;
    }

    m_all_tanks.push_back(std::move(aiTank1));
    m_all_tanks.push_back(std::move(aiTank2));
    m_all_tanks.push_back(std::move(aiTank3));
    m_all_tanks.push_back(std::move(aiTank4));
    std::cout << "AI tanks (type 'ai_default') loaded successfully." << std::endl;
}

// 将新创建的子弹添加到游戏世界中
void Game::addBullet(std::unique_ptr<Bullet> bullet) {
    if(bullet){ // 确保指针有效
        m_bullets.push_back(std::move(bullet)); // 添加到子弹列表
    }
}

// 游戏主循环
void Game::run() {
    while (window.isOpen()) { // 当窗口打开时持续循环
        sf::Time deltaTime = clock.restart(); // 获取自上次调用以来经过的时间，用于帧同步

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
                if(m_playerTankPtr && !m_playerTankPtr->isDestroyed() && m_playerTankPtr->canShoot()){ // 玩家坦克存在、未被摧毁且可以射击
                    std::unique_ptr<Bullet> new_Bullet = m_playerTankPtr->shoot(*this); // 玩家坦克射击，shoot 方法现在需要 Game&
                    if(new_Bullet){
                        addBullet(std::move(new_Bullet)); // 添加子弹到游戏世界
                        // std::cout << "Player bullet created" << std::endl;
                    }
                    // else {
                    // std::cout << "Player bullet not created (e.g., on cooldown)" << std::endl;
                    // }
                }
            }
        }
    }

    // 处理玩家坦克的持续按键移动
    if(m_playerTankPtr && !m_playerTankPtr->isDestroyed()){
        // float speed = m_playerTankPtr->getSpeed(); // 获取玩家坦克的当前速度
        // 应该使用坦克的 m_speed 成员，而不是固定值，因为可能有速度buff
        float distance = m_playerTankPtr->getSpeed() * deltaTime.asSeconds(); // 根据速度和时间计算移动距离

        // 注意：Tank::setDirection 和 Tank::move 现在需要 Game& game 参数
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
        {
            sf::Vector2f targetLocation = m_playerTankPtr->get_position() + sf::Vector2f(-distance, 0.f);
            m_playerTankPtr->setDirection(Direction::LEFT, *this); // 设置方向，传递 Game 引用
            m_playerTankPtr->move(targetLocation, m_map);          // 移动
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
                // AI移动逻辑
                if (!aiTankPtr->isMoving()) { // 如果AI当前没有在进行格子间移动
                    aiTankPtr->decideNextAction(m_map, m_playerTankPtr); // AI决策下一步行动
                }
                aiTankPtr->updateMovementBetweenTiles(dt, m_map); // 更新AI的格子间平滑移动

                // AI自动射击逻辑
                if (aiTankPtr->canShootAI()) { // 检查AI是否可以射击 (基于其特定冷却)
                    std::unique_ptr<Bullet> newBullet = aiTankPtr->shoot(*this); // AI射击，shoot 方法需要 Game&
                    if (newBullet) {
                        addBullet(std::move(newBullet));
                        aiTankPtr->resetShootTimerAI(); // 重置AI的射击计时器
                        // std::cout << "AI Tank (type '" << aiTankPtr->getTankType() << "') shot a bullet." << std::endl;
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
                    // std::cout << "Collision between Tank " << i << " (type '" << m_all_tanks[i]->getTankType() << "') and Tank " << j << " (type '" << m_all_tanks[j]->getTankType() << "')" << std::endl;
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
                    // TODO: 可以添加子弹所有者检查，避免自伤 (bullet->getOwner() != tankPtr.get())
                    if (bullet->getBounds().intersects(tankPtr->getBounds())) { // 检查碰撞
                        tankPtr->takeDamage(bullet->getDamage()); // 坦克受损
                        bullet->setIsAlive(false);                // 子弹失效
                        // std::cout << "Bullet hit Tank (type '" << tankPtr->getTankType() << "')." << std::endl;
                        break; //一颗子弹通常只击中一个目标
                    }
                }
            }
        }
    }
    //    b. 子弹与地图的碰撞
    for (auto& bullet : m_bullets) {
        if (bullet && bullet->isAlive()) {
            sf::Vector2f bulletPos = bullet->getPosition();
            // 将子弹的像素位置转换为地图的瓦片索引
            int tileX = static_cast<int>(bulletPos.x / m_map.getTileWidth());
            int tileY = static_cast<int>(bulletPos.y / m_map.getTileHeight());

            if (tileX >= 0 && tileX < m_map.getMapWidth() && tileY >= 0 && tileY < m_map.getMapHeight()) { // 确保在地图边界内
                if (!m_map.isTileWalkable(tileX, tileY)) { // 如果子弹击中不可通行的瓦片
                    // std::cout << "Bullet hit map tile (" << tileX << ", " << tileY << ")" << std::endl;
                    bullet->setIsAlive(false); // 子弹失效
                    // 可选：处理瓦片被破坏的逻辑 (m_map.destroyTile(tileX, tileY);)
                }
            } else { // 子弹飞出地图边界
                bullet->setIsAlive(false);
                // std::cout << "Bullet went off map and was destroyed." << std::endl;
            }
        }
    }

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
                                             // std::cout << "Tank (type '" << tank_to_check->getTankType() << "') at ("
                                             //           << tank_to_check->get_position().x << ", " << tank_to_check->get_position().y
                                             //           << ") marked for removal." << std::endl;
                                             if (m_playerTankPtr == tank_to_check.get()) {
                                                 // 如果被移除的是玩家坦克，m_playerTankPtr 需要在之后被置空
                                                 // 这里只做标记，实际置空在下面进行
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
    } else { // 如果 m_playerTankPtr 本来就是 nullptr (例如，游戏开始时就没有玩家，或者玩家已被移除)
        // 可能需要检查游戏是否应该结束
        if (m_all_tanks.empty() && state == GameState::Playing1P) { // 简单示例：如果所有坦克都没了，且是单人模式
            std::cout << "No tanks left. Potentially game over." << std::endl;
            // state = GameState::GameOver;
        }
    }

    // 8. 其他游戏逻辑 (例如检查游戏胜利/失败条件，生成新的敌人/道具等)
    if (m_playerTankPtr == nullptr && state == GameState::Playing1P) {
        std::cout << "Player is null, game over!" << std::endl;
        // state = GameState::GameOver; // 切换到游戏结束状态
        // window.close(); // 简单处理：直接关闭窗口
    }
    // 检查基地是否被摧毁 (假设基地是一个特殊的瓦片或对象)
    // sf::Vector2i basePos = m_map.getBaseTileCoordinate();
    // if (basePos.x != -1 && !m_map.isTileWalkable(basePos.x, basePos.y)) { // 假设基地瓦片被破坏后会变成不可行走
    //     bool base_destroyed_by_tile_change = true; // 更复杂的逻辑可能需要检查特定瓦片ID
    //     // if(m_map.getTileType(basePos.x, basePos.y) == TileType::DestroyedBase) {
    //     //    state = GameState::GameOver;
    //     //    std::cout << "Base destroyed! Game Over." << std::endl;
    //     // }
    // }
}

// 将游戏世界渲染到窗口上
void Game::render() {
    window.clear(sf::Color(100, 100, 100)); // 清屏，使用深灰色背景

    // 绘制地图，将 Game 对象自身引用传递给 Map 类
    m_map.draw(window, *this); // Map::draw 现在需要 Game& game 参数

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

    // (可选) 绘制道具
    // for (const auto &tool : m_tools) {
    //    if (tool && tool->isActive()) {
    //        tool->draw(window);
    //    }
    // }

    // (可选) 绘制UI元素 (得分、生命值等)
    // sf::Font font;
    // if (font.loadFromFile("path/to/your/font.ttf")) { // 确保你有字体文件
    //     sf::Text scoreText("Score: " + std::to_string(score), font, 24);
    //     scoreText.setFillColor(sf::Color::White);
    //     scoreText.setPosition(10, 10);
    //     window.draw(scoreText);

    //     sf::Text lifeText("Lives: " + std::to_string(life), font, 24);
    //     lifeText.setFillColor(sf::Color::White);
    //     lifeText.setPosition(window.getSize().x - 100, 10);
    //     window.draw(lifeText);
    // }


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

        // 计算推开向量 (从 tank2 指向 tank1，即 tank1 被推开的方向)
        sf::Vector2f pushDirection = pos1 - pos2;

        // 如果坦克中心重合或非常接近，给一个默认推开方向
        if (pushDirection.x == 0 && pushDirection.y == 0) {
            pushDirection = sf::Vector2f(0.f, -1.f); // 默认向上推开 tank1
        }

        // 归一化推开方向
        float length = std::sqrt(pushDirection.x * pushDirection.x + pushDirection.y * pushDirection.y);
        if (length != 0) {
            pushDirection /= length;
        }

        // 计算推开的幅度 (重叠量的一半，加上一点微小偏移以确保分开)
        // 使用交集宽度和高度中较小者作为基础，避免在一个方向上过度推开
        float pushMagnitude = (std::min(intersection.width, intersection.height) / 2.0f) + 0.5f; // 增加一点偏移

        sf::Vector2f moveOffset = pushDirection * pushMagnitude;

        // 尝试将两个坦克沿相反方向各推开一半的重叠量
        // Tank::move 方法内部会进行地图碰撞检测
        tank1->move(pos1 + moveOffset, m_map);
        tank2->move(pos2 - moveOffset, m_map); // tank2 向相反方向移动

        // (可选) 如果是AI坦克，碰撞后可能需要重新规划路径
        AITank* ai1 = dynamic_cast<AITank*>(tank1);
        AITank* ai2 = dynamic_cast<AITank*>(tank2);
        if (ai1 && ai1->isMoving()) {
            // ai1->forceReplanPath(); // 假设有这样的方法
        }
        if (ai2 && ai2->isMoving()) {
            // ai2->forceReplanPath();
        }
    }
}
