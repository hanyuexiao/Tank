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
    auto aiTank1 = std::make_unique<AITank>(sf::Vector2f(1230.f, 300.f), Direction::UP, 30.f);
    auto aiTank2 = std::make_unique<AITank>(sf::Vector2f(600.f, 300.f), Direction::UP, 30.f);
    auto aiTank3 = std::make_unique<AITank>(sf::Vector2f(700.f, 300.f), Direction::UP, 30.f);
    auto aiTank4 = std::make_unique<AITank>(sf::Vector2f(200.f, 300.f), Direction::UP, 30.f);

    // 将所有AI坦克先存入一个临时vector，方便统一处理
    std::vector<AITank*> ai_raw_pointers;
    if (auto ptr = dynamic_cast<AITank*>(aiTank1.get())) ai_raw_pointers.push_back(ptr);
    if (auto ptr = dynamic_cast<AITank*>(aiTank2.get())) ai_raw_pointers.push_back(ptr);
    if (auto ptr = dynamic_cast<AITank*>(aiTank3.get())) ai_raw_pointers.push_back(ptr);
    if (auto ptr = dynamic_cast<AITank*>(aiTank4.get())) ai_raw_pointers.push_back(ptr);

    sf::Vector2i baseTile = m_map.getBaseTileCoordinate();
    if (baseTile.x != -1 && baseTile.y != -1) {
        for (AITank* pAiTank : ai_raw_pointers) { // 遍历所有获取到的AI坦克指针
            if (pAiTank) {
                pAiTank->setStrategicTargetTile(baseTile); // 为每一个AI坦克设置目标
                std::cout << "AI Tank at (" << pAiTank->get_position().x << ", " << pAiTank->get_position().y
                          << ") target set to base." << std::endl;
            }
        }
    } else {
        std::cerr << "Game: Failed to get base tile coordinate for AI setup. All AI will idle." << std::endl;
        // 如果获取不到基地位置，所有AI都不会设置目标，因此都不会移动（除非你有其他逻辑）
    }

    m_all_tanks.push_back(std::move(aiTank1));
    m_all_tanks.push_back(std::move(aiTank2));
    m_all_tanks.push_back(std::move(aiTank3));
    m_all_tanks.push_back(std::move(aiTank4));
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
    // 1. 更新所有坦克的基础状态 (动画、通用计时器等)
    //    这个循环可以保持，它调用的是 Tank::update() 或其派生类的覆盖版本
    for(auto& tankPtr : m_all_tanks) {
        if(tankPtr && !tankPtr->isDestroyed()) { // 移除了多余的 tankPtr 检查
            tankPtr->update(dt); // 这会调用 PlayerTank::update 或 AITank::update
        }
    }

    // 2. 更新AI坦克的特定逻辑 (移动决策、格子间移动、自动射击)
    for (auto& tankPtr : m_all_tanks) {
        if (tankPtr && !tankPtr->isDestroyed()) {
            AITank* aiTankPtr = dynamic_cast<AITank*>(tankPtr.get());
            if (aiTankPtr) { // 这是AI坦克
                // AI移动逻辑
                if (!aiTankPtr->isMoving()) {
                    aiTankPtr->decideNextAction(m_map, m_playerTankPtr);
                }
                aiTankPtr->updateMovementBetweenTiles(dt, m_map);

                // AI自动射击逻辑
                if (aiTankPtr->canShootAI()) { // 使用AITank特定的冷却检查
                    // 可选：增加一些条件，比如AI是否看到玩家，或者是否处于攻击姿态等
                    // if (aiTankPtr->hasLineOfSightTo(m_playerTankPtr) && !aiTankPtr->isMoving()) {
                    std::unique_ptr<Bullet> newBullet = aiTankPtr->shoot(*this); // 调用基类的shoot或AITank重写的shoot
                    if (newBullet) {
                        m_bullets.push_back(std::move(newBullet));
                        aiTankPtr->resetShootTimerAI(); // 重置AI的计时器并生成新CD
                        std::cout << "AI Tank shot a bullet." << std::endl;
                    }
                    // }
                }
            }
            // PlayerTank的移动和射击在 Handling_events 中处理，这里不需要特别为PlayerTank做什么
        }
    }

    // 阶段2: 处理实体间的碰撞 (坦克 vs 坦克)
    for (size_t i = 0; i < m_all_tanks.size(); ++i) {
        for (size_t j = i + 1; j < m_all_tanks.size(); ++j) {
            // 确保两个坦克指针都有效，并且坦克都还存活
            if (m_all_tanks[i] && !m_all_tanks[i]->isDestroyed() &&
                m_all_tanks[j] && !m_all_tanks[j]->isDestroyed()) {

                if (m_all_tanks[i]->getBounds().intersects(m_all_tanks[j]->getBounds())) {
                    // 发生碰撞了！
                    std::cout << "Collision between Tank " << i << " and Tank " << j << std::endl;

                    // 在这里调用碰撞解决函数
                    resolveTankCollision(m_all_tanks[i].get(), m_all_tanks[j].get());
                }
            }
        }
    }
    // 3. 更新所有子弹的状态 (移动)
    for (auto& bullet : m_bullets) {
        if (bullet && bullet->isAlive()) {
            bullet->update(dt);
        }
    }

    // 4. 处理碰撞逻辑
    //    a. 子弹与坦克的碰撞
    for (auto& bullet : m_bullets) {
        if (bullet && bullet->isAlive()) {
            for (auto& tankPtr : m_all_tanks) {
                if (tankPtr && !tankPtr->isDestroyed()) { // 只与存活的坦克碰撞
                    if (bullet->getBounds().intersects(tankPtr->getBounds())) {
                        // 考虑添加子弹拥有者检查，避免自伤
                        tankPtr->takeDamage(bullet->getDamage()); // 坦克受损，可能isDestroyed()会变为true
                        bullet->setIsAlive(false);
                        break;
                    }
                }
            }
        }
    }
    //    b. 子弹与地图的碰撞
// --- 子弹与地图的碰撞检测 ---
    for (auto& bullet : m_bullets) {
        if (bullet && bullet->isAlive()) { // 检查子弹是否有效且存活
            sf::Vector2f bulletPos = bullet->getPosition(); // 获取子弹当前位置
            // 将子弹的像素位置转换为地图的瓦片索引
            int tileX = static_cast<int>(bulletPos.x / m_map.getTileWidth());
            int tileY = static_cast<int>(bulletPos.y / m_map.getTileHeight());

            // 在检查瓦片之前，确保瓦片坐标在地图边界内
            if (tileX >= 0 && tileX < m_map.getMapWidth() && tileY >= 0 && tileY < m_map.getMapHeight()) {
                if (!m_map.isTileWalkable(tileX, tileY)) { // 假设 isTileWalkable 返回 false 表示该瓦片对子弹是障碍物
                    // 你可能需要一个更具体的检查。例如，某些对坦克不可通行的瓦片，子弹可能可以穿透，反之亦然。
                    // 或者某些瓦片是可被子弹破坏的。
                    std::cout << "子弹击中地图瓦片 (" << tileX << ", " << tileY << ")" << std::endl;
                    bullet->setIsAlive(false); // 子弹失效
                    // 可选：处理瓦片被破坏的逻辑
                    // m_map.destroyTile(tileX, tileY);
                }
            }
        }
    }

    // 5. 清理不再存活的实体
    //    a. 清理子弹 (那些 isAlive() 返回 false 的)
    m_bullets.erase(std::remove_if(m_bullets.begin(), m_bullets.end(), [](const std::unique_ptr<Bullet>& b) {
        return !b || !b->isAlive();
    }), m_bullets.end());

    //    b. 清理坦克 (那些 isDestroyed() 返回 true 的)
    //       这就是你关注的“擦除”逻辑，确保它在所有伤害判定之后
    m_all_tanks.erase(std::remove_if(m_all_tanks.begin(), m_all_tanks.end(),
                                     [&](const std::unique_ptr<Tank>& tank_to_check) {
                                         bool should_remove = tank_to_check && tank_to_check->isDestroyed();
                                         if (should_remove && m_playerTankPtr == tank_to_check.get()) {
                                             // 如果被移除的是玩家坦克，需要特殊处理 m_playerTankPtr
                                             // 但 m_playerTankPtr 的置空应该在 erase 完成之后进行，
                                             // 因为此时 tank_to_check 仍然是有效的（尽管将要被标记为移除）
                                             std::cout << "Player tank marked for removal." << std::endl;
                                         }
                                         return should_remove;
                                     }),
                      m_all_tanks.end());

    // 6. 在清理完坦克后，检查并更新 m_playerTankPtr (如果它指向的坦克已被移除)
    if (m_playerTankPtr) {
        bool player_found = false;
        for (const auto& tank_ptr : m_all_tanks) {
            if (tank_ptr.get() == m_playerTankPtr) {
                player_found = true;
                break;
            }
        }
        if (!player_found) {
            m_playerTankPtr = nullptr;
            std::cout << "Player tank has been removed. m_playerTankPtr is now nullptr." << std::endl;
            // 在这里可以添加游戏结束的逻辑，比如切换到 GameOver 状态
            // state = GameState::GameOver;
        }
    }

    // 7. 其他游戏逻辑 (例如检查游戏结束条件，生成新的敌人等)
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


void Game::resolveTankCollision(Tank* tank1, Tank* tank2) {
    if (!tank1 || !tank2) return;

    sf::FloatRect bounds1 = tank1->getBounds();
    sf::FloatRect bounds2 = tank2->getBounds();
    sf::FloatRect intersection;

    if (bounds1.intersects(bounds2, intersection)) {
        // 获取两个坦克当前的位置
        sf::Vector2f pos1 = tank1->get_position();
        sf::Vector2f pos2 = tank2->get_position();

        // 计算从 tank1 中心指向 tank2 中心的向量
        sf::Vector2f center1(bounds1.left + bounds1.width / 2, bounds1.top + bounds1.height / 2);
        sf::Vector2f center2(bounds2.left + bounds2.width / 2, bounds2.top + bounds2.height / 2);
        sf::Vector2f pushDirection = center1 - center2; // 推离的方向 (tank1 从 tank2 推开)

        // 归一化推离方向 (如果长度不为0)
        float length = std::sqrt(pushDirection.x * pushDirection.x + pushDirection.y * pushDirection.y);
        if (length != 0) {
            pushDirection /= length;
        } else {
            // 如果中心重合，随便给个方向，比如向上
            pushDirection = sf::Vector2f(0.f, -1.f);
        }

        // 计算推开的距离
        // 可以是重叠区域的一半宽度或高度，或者一个固定的小值
        // 这里用一个简化值：重叠宽度和高度中较小者的一半，再加一点点防止持续碰撞
        float pushMagnitude = std::min(intersection.width, intersection.height) / 2.0f + 0.1f;

        sf::Vector2f pushVector = pushDirection * pushMagnitude;

        // 尝试将 tank1 推开
        // 注意：这里的 Tank::move 是你现有的方法，它内部有地图碰撞检测。
        // 所以，如果推开导致撞墙，坦克可能不会移动那么多。
        // 我们只尝试移动一个坦克，或者各移动一半。为了简单，先只移动 tank1。
        // 或者更公平地，两个坦克都沿相反方向移动重叠量的一半。

        sf::Vector2f tank1NewPos = pos1 + pushVector;
        sf::Vector2f tank2NewPos = pos2 - pushVector; // tank2 向相反方向移动

        // 实际应用移动时，需要考虑这两个新的位置是否会导致新的碰撞 (与地图或其他坦克)
        // Tank::move 内部会处理与地图的碰撞。
        // 为了避免推入其他坦克，这个resolveTankCollision可能需要迭代几次，或者有更复杂的全局解决。
        // 简化处理：我们只移动，让Tank::move处理地图碰撞。

        // 保存旧位置，以防move完全失败 (例如AI坦克卡住时)
        sf::Vector2f oldPos1 = tank1->get_position();
        sf::Vector2f oldPos2 = tank2->get_position();

        // 实际移动坦克 (这里用的是坦克的set_position，然后让坦克的sprite也更新)
        // 更理想的方式是让坦克的move方法能够接受一个“尝试移动”并返回结果，
        // 或者有一个单独的 setPositionAndUpdateSprite 方法。
        // 你的 Tank::move 已经是基于目标位置的，所以我们可以用它。

        // 这里的逻辑需要小心，因为 Tank::move 是期望一个目标位置，
        // 而我们计算的是一个位移。
        // 更好的方法是修改 Tank 类，使其能被“推移”一个向量，
        // 或者直接设置其 m_position 和 m_sprite.setPosition，
        // 但这绕过了 Tank::move 的地图碰撞。

        // 一个折衷：如果 tank1 是玩家，优先移动 AI tank。
        // 或者，判断哪个坦克“更应该”移动 (例如，正在移动的那个)。

        // --- 简单粗暴的实现：两个坦克都尝试从碰撞中移开一点 ---
        // 这种方式可能导致连锁反应或抖动，需要精细调整。

        // 考虑坦克当前速度/方向，让“撞人”的坦克承担更多位移
        // 以下是一种非常基础的均分位移（但要注意方向）

        // 计算分离的向量，将 tank1 推离 tank2
        float overlapX = 0.f;
        float overlapY = 0.f;

        // 计算X和Y方向的重叠量
        if (intersection.width < intersection.height) { // 优先解决较小的重叠方向
            if (center1.x < center2.x) { // tank1 在 tank2 左边
                overlapX = -intersection.width;
            } else {
                overlapX = intersection.width;
            }
        } else {
            if (center1.y < center2.y) { // tank1 在 tank2 上边
                overlapY = -intersection.height;
            } else {
                overlapY = intersection.height;
            }
        }

        // 将两个坦克分别移开重叠量的一半
        // 注意：Tank::move 是基于“目标位置”的，所以我们需要计算新的目标位置
        // 并且 Tank::move 会做地图碰撞检测
        // 这里的 +0.1f 是为了确保它们完全分开
        tank1->move(pos1 + sf::Vector2f(overlapX / 2.0f + (overlapX > 0 ? 0.1f : (overlapX < 0 ? -0.1f : 0.f)),
                                        overlapY / 2.0f + (overlapY > 0 ? 0.1f : (overlapY < 0 ? -0.1f : 0.f))),
                    m_map); // 假设 Game 类能访问 m_map
        tank2->move(pos2 - sf::Vector2f(overlapX / 2.0f + (overlapX > 0 ? 0.1f : (overlapX < 0 ? -0.1f : 0.f)),
                                        overlapY / 2.0f + (overlapY > 0 ? 0.1f : (overlapY < 0 ? -0.1f : 0.f))),
                    m_map);

        // 对于AI坦克，这种碰撞可能意味着它需要重新规划路径
        AITank* ai1 = dynamic_cast<AITank*>(tank1);
        AITank* ai2 = dynamic_cast<AITank*>(tank2);
        if (ai1 && ai1->isMoving()) { // 如果AI1正在移动并发生了碰撞
            // ai1->stopCurrentMoveAndReplan(); // 假设有这样的方法
            // 简单处理：强制停止当前格子移动，让下一帧重新decideNextAction
            // 这需要AITank类有类似 setMovingToNextTile(false) 的方法，或者在decideNextAction中处理
        }
        if (ai2 && ai2->isMoving()) {
            // ai2->stopCurrentMoveAndReplan();
        }
    }
}


