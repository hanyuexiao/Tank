#include "game.h"
#include "AITank.h"
#include "PlayerTank.h"

Game::Game(): window(sf::VideoMode(1500, 750), "Tank Battle!"),
            state(GameState::MainMenu),
            score(0),
            life(3),
            m_map(){
    // 构造函数
    // 初始化窗口
    std::cout << "Game constructor called." << std::endl;
}

Game::~Game() {
    std::cout << "Game destructor called." << std::endl;
}

void Game::init() {
    std::cout << "Game::init() called." << std::endl;
    window.setFramerateLimit(60);
    window.setVerticalSyncEnabled(true);
    load_date();
    // 初始化游戏

    // 加载地图
    if(!m_map.load()){
        std::cerr << "CRITICAL ERROR: Failed to load map in Game::init()" << std::endl;
        window.close();
        return;
    }
    std::cout << "Map loaded successfully." << std::endl;

    // 加载玩家坦克
    auto player = std::make_unique<PlayerTank>(sf::Vector2f (300.f, 375.f),Direction::UP);
    m_playerTankPtr = player.get();
    m_all_tanks.push_back(std::move(player));
    std::cout << "Player tank loaded successfully." << std::endl;

    //加载AI坦克
    auto aiTankOwner = std::make_unique<AITank>(sf::Vector2f(750.f, 200.f), Direction::DOWN, 60.f); // 示例初始位置和速度
    AITank* pAiTank = dynamic_cast<AITank*>(aiTankOwner.get());

    if (pAiTank) {
        sf::Vector2i baseTile = m_map.getBaseTileCoordinate();
        if (baseTile.x != -1 && baseTile.y != -1) { // 确保获取到有效的大本营位置
            pAiTank->setStrategicTargetTile(baseTile);
        } else {
            std::cerr << "Game: Failed to get base tile coordinate for AI setup." << std::endl;
            // 可以给AI一个默认巡逻点或让其待机
            // pAiTank->setStrategicTargetTile(sf::Vector2i(m_map.getMapWidth()/2, m_map.getMapHeight()/2)); // 例如，地图中心
        }
    }
    m_all_tanks.push_back(std::move(aiTankOwner));
    // 创建窗口
    // 加载资源
    // 初始化游戏状态
    // ...
}

void Game::addBullet(std::unique_ptr<Bullet> bullet) {
    if(bullet){
        m_bullets.push_back(std::move(bullet));
    }
}


void Game::run() {
    // 游戏主循环
    while (window.isOpen()) {
        sf::Time deltaTime = clock.restart();
        //1. 处理事件
        Handling_events(deltaTime);

        // 2. 更新游戏逻辑 (现在是空的)
        update(deltaTime);

        // 3. 渲染画面
        render();
    }
}
        //处理玩家输入
void Game::Handling_events(sf::Time deltaTime) {
        sf::Event event{};
        while (window.pollEvent(event)) {
            if(event.type == sf::Event::Closed){
                window.close();
            }
            if( event.type == sf::Event::KeyPressed){
                if(event.key.code == sf::Keyboard::Space){
                    if(m_playerTankPtr && m_playerTankPtr->canShoot()){
                        std::unique_ptr<Bullet> new_Bullet = m_playerTankPtr->shoot(*this);
                        if(new_Bullet){
                            m_bullets.push_back(std::move(new_Bullet));
                            std::cout << "Bullet created" << std::endl;
                        }
                        else {
                            std::cout << "Bullet not created" << std::endl;
                        }
                    }
                }
            }

        }

        if(m_playerTankPtr){

            float speed = 100.f;
            float distance = speed * deltaTime.asSeconds();
            bool playerMoved = false;
            Direction NewDirection = m_playerTankPtr->get_Direction();

            if(sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
            {
                sf::Vector2f targetLocation = m_playerTankPtr->get_position() + sf::Vector2f(-distance, 0); // 获取当前位置并计算目标位置
                m_playerTankPtr->setDirection(Direction::LEFT);
                m_playerTankPtr->move(targetLocation, m_map);
                playerMoved = true;
                NewDirection = Direction::LEFT;
            }
            else if(sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
            {
                sf::Vector2f targetLocation = m_playerTankPtr ->get_position() + sf::Vector2f(distance, 0);
                m_playerTankPtr ->setDirection(Direction::RIGHT);
                m_playerTankPtr ->move(targetLocation, m_map);
                playerMoved = true;
                NewDirection = Direction::RIGHT;
            }
            else if(sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
            {
                sf::Vector2f targetLocation = m_playerTankPtr ->get_position() + sf::Vector2f(0, -distance);
                m_playerTankPtr ->setDirection(Direction::UP);
                m_playerTankPtr ->move(targetLocation, m_map);
                playerMoved = true;
                NewDirection = Direction::UP;
            }
            else if(sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
            {
                sf::Vector2f targetLocation = m_playerTankPtr ->get_position() + sf::Vector2f(0, distance);
                m_playerTankPtr ->setDirection(Direction::DOWN);
                m_playerTankPtr ->move(targetLocation, m_map);
                playerMoved = true;
                NewDirection = Direction::DOWN;
            }
        }
}

void Game::end() {
    // 结束游戏
    // 保存游戏状态
    // 清理资源
    // 关闭窗口
}

void Game::update(sf::Time dt) {
    // 更新所有坦克 (玩家和AI)
// 阶段1: 更新所有坦克的决策和意图移动 (调用 decideNextAction, updateMovementBetweenTiles)
    for (auto& tankPtr : m_all_tanks) {
        AITank* aiTankPtr = dynamic_cast<AITank*>(tankPtr.get());
        if (aiTankPtr) {
            if (!aiTankPtr->isMoving()) {
                aiTankPtr->decideNextAction(m_map, m_playerTankPtr);
            }
            aiTankPtr->updateMovementBetweenTiles(dt, m_map); // 内部调用 Tank::move
        }
        // 玩家坦克的移动逻辑也应该在这里，它也会调用 Tank::move
        // playerTankPtr->handleInputAndMove(dt, m_map);

        tankPtr->update(dt); // Tank::update (动画等)
    }

// 阶段2: 处理实体间的碰撞 (坦克 vs 坦克)
    for (size_t i = 0; i < m_all_tanks.size(); ++i) {
        for (size_t j = i + 1; j < m_all_tanks.size(); ++j) {
            if (m_all_tanks[i]->getBounds().intersects(m_all_tanks[j]->getBounds())) {
                // 处理坦克 i 和坦克 j 之间的碰撞
                // 例如，将它们的位置回退到碰撞前的位置，或者稍微推开
                // AI坦克可能需要因为这种碰撞而重新规划路径
                // resolveTankCollision(m_allTanks[i].get(), m_allTanks[j].get());
            }
        }
    }

// 阶段3: 更新子弹，处理子弹与坦克的碰撞，子弹与地图的碰撞

    // 更新子弹
    for (auto& bullet : m_bullets) {
        if (bullet && bullet->isAlive()) {
            bullet->update(dt);
        }
    }
    // 移除不再活动的子弹
    m_bullets.erase(std::remove_if(m_bullets.begin(), m_bullets.end(), [](const std::unique_ptr<Bullet>& bullet) {
        return !bullet || !bullet->isAlive();
    }), m_bullets.end());

    // 子弹出界检查
    for (auto& bullet : m_bullets) {
        if (bullet && bullet->isAlive()) {
            sf::Vector2f pos = bullet->getPosition();
            sf::FloatRect bounds = bullet->getBounds(); // 假设Bullet有getBounds
            if (pos.x + bounds.width < 0 || pos.x > window.getSize().x || pos.y + bounds.height < 0 || pos.y > window.getSize().y) {
                bullet->setIsAlive(false);
                std::cout << "Bullet out of bounds, marked as not alive" << std::endl;
            }
        }
    }
    // 其他游戏逻辑，如碰撞检测等...
}

void Game::render() {
    window.clear(sf::Color::Green); // 清屏，换个颜色试试看
    m_map.draw(window);
    // 在这里绘制游戏元素... (现在是空的)
    for (const auto &tank: m_all_tanks) {
        tank->draw(window);
    }

    for (const auto &bullet: m_bullets) {
        if (bullet) {
            bullet->draw(window);
        }
    }
    window.display();
}

void Game::load_date()
{
    std::cout << "Game::load_date() - Loading assists...." << std::endl;

    std::string bulletBasePath = R"(C:\Users\admin\CLionProjects\Tanks\Bullet_img\)";

    sf::Texture tempTexture;

    if(!tempTexture.loadFromFile(bulletBasePath + "bullet_up.png")) {
        std::cerr << "Game::load_date() - Failed to load bullet_up.png" << std::endl;
    }else{
        m_bullet_textures[Direction::UP] = tempTexture;
    }
    if(!tempTexture.loadFromFile(bulletBasePath + "bullet_down.png")) {
        std::cerr << "Game::load_date() - Failed to load bullet_down.png" << std::endl;
    }else{
        m_bullet_textures[Direction::DOWN] = tempTexture;
    }
    if(!tempTexture.loadFromFile(bulletBasePath + "bullet_left.png")) {
        std::cerr << "Game::load_date() - Failed to load bullet_left.png" << std::endl;
    }else{
        m_bullet_textures[Direction::LEFT] = tempTexture;
    }
    if(!tempTexture.loadFromFile(bulletBasePath + "bullet_right.png")) {
        std::cerr << "Game::load_date() - Failed to load bullet_right.png" << std::endl;
    }
    else{
        m_bullet_textures[Direction::RIGHT] = tempTexture;
    }

    if (m_bullet_textures.size() < 4){
        std::cerr << "Game::load_date() - Failed to load all bullet textures" << std::endl;
    }else{
        std::cout << "Game::load_date() - All bullet textures loaded" << std::endl;
    }
}


//it 读入有误
const sf::Texture &Game::getBulletTexture(Direction dir) const {
    auto it = m_bullet_textures.find(dir);
    if (it != m_bullet_textures.end()){
        return it->second;
    }else {
        std::cerr << "Game::getBulletTexture() - Failed to find bullet texture" << static_cast<int>(dir)<<" not fount in m_bullet_textures map!" <<std::endl;

        if(!m_bullet_textures.empty()){
            std::cerr << "Game::getBulletTexture() - Returning first bullet texture" << std::endl;
            return m_bullet_textures.begin()->second;
        }else{
            std::cerr << "Game::getBulletTexture() - m_bullet_textures is empty" << std::endl;
            static sf::Texture emptyTexture;
            return emptyTexture;
        }
    }
}



