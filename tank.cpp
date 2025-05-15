#include "tank.h"
#include "Bullet.h"
#include "Game.h"

Tank::Tank(sf::Vector2f startPosition, Direction startDirection,float speed,int frameWidth, int frameHeight,int iniHealth) :
        m_position(startPosition),
        m_direction(startDirection),
        m_currentFrame(0),
        m_frameWidth(frameWidth),         // 使用传入的参数
        m_frameHeight(frameHeight),       // 使用传入的参数
        m_shootCooldown(sf::seconds(0.5f)),
        m_shootTimer(sf::Time::Zero),
        m_speed(speed),// *** 确保 m_shootTimer 在这里初始化 ***
        m_health(iniHealth),
        m_MaxHealth(iniHealth),
        m_Destroyed(false)
{
    loadTextures(); // 加载所有方向的纹理
    // 设置初始纹理和位置
    if (!m_textures.empty() && (static_cast<int>(m_direction) * 2 + m_currentFrame) < m_textures.size()) {
        m_sprite.setTexture(m_textures[static_cast<int>(m_direction) * 2 + m_currentFrame]);
    } else {
        // 最好有错误处理或日志记录
        std::cerr << "Tank Constructor Error: Initial texture could not be set for direction " << static_cast<int>(m_direction) << ". Textures loaded: " << m_textures.size() << std::endl;
        // 比如，如果纹理加载失败，m_textures可能是空的
    }
    //将tank原点设置在图片中心
    if(m_frameWidth>0 && m_frameHeight >0){
        m_sprite.setOrigin(m_frameWidth/2.f, m_frameHeight/2.f);
        // 调试打印：
        std::cout << "Tank Type (debug): " << typeid(*this).name() // 这会打印出对象类型，比如 PlayerTank 或 AITank
                  << " Set Origin to: (" << m_sprite.getOrigin().x
                  << ", " << m_sprite.getOrigin().y << ")"
                  << " with frameWidth: " << m_frameWidth
                  << " frameHeight: " << m_frameHeight << std::endl;
    }else{
        std::cerr << "Tank Constructor Error: Frame width or height is zero." << std::endl;
    }

    m_sprite.setPosition(m_position);
}



void Tank::loadTextures() {
    std::vector<std::string> directions = { "left", "right", "up", "down" }; // 方向字符串
    for (const auto& dir : directions) {
        for (int i = 0; i < 2; ++i) { // 假设每个方向 2 帧
            sf::Texture texture;
            if (!texture.loadFromFile("C:\\Users\\admin\\CLionProjects\\Tanks\\tank/tank_" + dir + "_" + std::to_string(i) + ".png")) {
                // 错误处理
                std::cerr << "Error loading texture: " << "tank/tank_" + dir + "_" + std::to_string(i) + ".png" << std::endl;
            }
            m_textures.push_back(texture);
        }
    }
}

void Tank::draw(sf::RenderWindow& window) {
    window.draw(m_sprite);

    sf::CircleShape centerDot(3.f);
    centerDot.setFillColor(sf::Color::Red);

    centerDot.setOrigin(3.f, 3.f);
    centerDot.setPosition(m_position);

    window.draw(centerDot);
}

void Tank::setDirection(Direction dir) {
    if (m_direction != dir) {
        m_direction = dir;
        m_currentFrame = 0;
        m_sprite.setTexture(m_textures[static_cast<int>(m_direction) * 2 + m_currentFrame]); // 切换纹理
    }
}

void Tank::update(sf::Time dt) {
    // 动画更新逻辑 (如果需要)
    m_currentFrame = (m_currentFrame + 1) % 2; // 切换帧
    if(!m_textures.empty()){
        m_sprite.setTexture(m_textures[static_cast<int>(m_direction) * 2 + m_currentFrame]);
    }

    if(m_shootTimer < m_shootCooldown){
        m_shootTimer += dt;
    }
}

void Tank::move(sf::Vector2f targetPosition, const Map& map) {
    // --- 步骤 1: 获取坦克在 *目标位置* 的边界框 ---
    // 创建一个临时精灵副本，将其移动到 targetPosition，然后获取其边界框
    // 这是为了确保我们是基于“如果坦克移动到那里”的情况来做碰撞检测
    sf::Sprite prospectiveSprite = m_sprite; // 复制当前坦克的精灵状态

    // !! 关键: 确保 prospectiveSprite 使用的是正确的纹理，
    // !! 特别是如果调用 move 之前，坦克的方向 (m_direction) 和对应的纹理 (m_sprite.setTexture) 已经更新了。
    // !! 如果 m_sprite 已经是最新方向的纹理，直接复制就行。
    // !! 如果不是，或者不确定，可以根据 m_direction 重新设置 prospectiveSprite 的纹理：
    // prospectiveSprite.setTexture(m_textures[static_cast<int>(m_direction) * 2 + m_currentFrame]); // 假设每方向2帧

    prospectiveSprite.setPosition(targetPosition); // 将副本精灵设置到目标位置
    sf::FloatRect tankBoundsAtTarget = prospectiveSprite.getGlobalBounds(); // 获取在这个目标位置的全局边界框

    // --- 调试打印：输出目标位置和计算出的预期边界框 ---
    std::cout << "Tank::move want to move: (" << targetPosition.x << ", " << targetPosition.y << ")" << std::endl;
    // 打印当前坦克的 m_direction 枚举值，方便追踪
    // 你可能需要根据你的 Direction 枚举定义来决定如何打印，static_cast<int> 是一个简单方法
    std::cout << "  hope direction: (m_direction): " << static_cast<int>(m_direction) << std::endl;
    // --- 步骤 2: 计算该边界框覆盖的地图瓦片范围 ---
    int tileW = map.getTileWidth();   // 从 Map 对象获取瓦片宽度
    int tileH = map.getTileHeight();  // 从 Map 对象获取瓦片高度

    // 确保 tileW 和 tileH 不是0，避免除以0的错误
    if (tileW <= 0 || tileH <= 0) {
        std::cerr << "Error! Width: " << tileW << ", Height: " << tileH << std::endl;
        return; // 或者其他错误处理
    }

    // 计算起始和结束的瓦片索引
    // 注意：对于结束索引 (endX, endY)，从宽度/高度中减去一个极小值 (如 0.001f)
    //       可以帮助更精确地处理当坦克边缘正好落在瓦片边界上的情况，
    //       防止因浮点数精度问题或整数截断而错误地多检测一个瓦片。
    int startX = static_cast<int>(tankBoundsAtTarget.left / tileW);
    int startY = static_cast<int>(tankBoundsAtTarget.top / tileH);
    int endX = static_cast<int>((tankBoundsAtTarget.left + tankBoundsAtTarget.width - 0.001f) / tileW);
    int endY = static_cast<int>((tankBoundsAtTarget.top + tankBoundsAtTarget.height - 0.001f) / tileH);



    // --- 步骤 3: 检查目标范围内的所有瓦片是否可通行 ---
    bool canMove = true;
    // 添加一个检查，确保计算出的瓦片范围是有效的 (例如，坦克不是完全在瓦片外部)
    if (endX < startX || endY < startY) {
        // 这个情况可能不应该发生，如果发生了，可能 tankBoundsAtTarget 或 tileW/H 有问题
        std::cout << "  警告: 检查的瓦片范围无效 (end < start)!" << std::endl;
        // 根据你的游戏逻辑，这里可能也应该视为 canMove = false;
    }

    for (int y_tile = startY; y_tile <= endY; ++y_tile) {
        for (int x_tile = startX; x_tile <= endX; ++x_tile) {
            // 调用 Map 对象的 isTileWalkable 来判断
            // Map::isTileWalkable 内部应该有它自己的边界检查，
            // 并且根据 m_layout[y_tile][x_tile] 的值来判断
            bool walkable = map.isTileWalkable(x_tile, y_tile);

            if (!walkable) {
                canMove = false;
                break; // 一旦遇到不可走的瓦片，就不需要再检查了
            }
        }
        if (!canMove) {
            break; // 跳出外层循环
        }
    }

    // --- 步骤 4: 如果所有检查的瓦片都可通行，则更新坦克位置 ---
    if (canMove) {
        m_position = targetPosition;       // 更新坦克的逻辑位置
        m_sprite.setPosition(m_position);  // 更新精灵的绘制位置
    }
    // 如果 !canMove，坦克的位置 (m_position 和 m_sprite 的位置) 保持不变

}

std::unique_ptr<Bullet> Tank::shoot(Game& gameInstance) { // 接收 Game 对象的引用
    if(m_shootTimer < m_shootCooldown){
        std::cout << "Tank::shoot() called too frequently. Timer:"
            << m_shootTimer.asSeconds() << "s/"
            << m_shootCooldown.asSeconds() << "s" << std::endl;
        return nullptr; // 如果冷却时间还没到，返回空指针
    }
    Direction currentTankDir = get_Direction(); // 获取坦克当前面向的方向
    m_shootTimer = sf::Time::Zero; // 重置射击计时器

    // 1. 根据坦克方向确定子弹的飞行方向向量 (flyVec)
    sf::Vector2f flyVec;
    switch (currentTankDir) {
        case Direction::UP:    flyVec = sf::Vector2f(0.f, -1.f); break;
        case Direction::DOWN:  flyVec = sf::Vector2f(0.f, 1.f);  break;
        case Direction::LEFT:  flyVec = sf::Vector2f(-1.f, 0.f); break;
        case Direction::RIGHT: flyVec = sf::Vector2f(1.f, 0.f);  break;
        default:
            std::cerr << "Tank::shoot() - Invalid tank direction encountered!" << std::endl;
            return nullptr; // 无效方向，不发射子弹
    }

    // 2. 获取对应方向的子弹纹理
    //    这里我们假设 gameInstance.getBulletTexture() 在找不到纹理时有健壮的错误处理
    //    或者保证在调用前所有纹理都已加载。
    const sf::Texture& bulletTexture = gameInstance.getBulletTexture(currentTankDir);

    // 获取子弹纹理的尺寸，用于更精确地计算起始位置和设置子弹精灵原点
    // 如果所有方向的子弹图片尺寸都一样，这会简单很多
    sf::Vector2u bulletTextureSize = bulletTexture.getSize();
    float bulletWidth = static_cast<float>(bulletTextureSize.x);
    float bulletHeight = static_cast<float>(bulletTextureSize.y);

    // 3. 计算子弹的精确起始位置 (bulletStartPos)
    //    目标是让子弹从坦克炮管口发射，并且其自身的中心点位于炮管口的中心线上。
    sf::Vector2f bulletStartPos = get_position(); // 获取坦克当前位置 (通常是左上角)
    float tankHalfWidth = bulletWidth / 2.f;
    float tankHalfHeight = bulletHeight /2.f;

    // 偏移因子，让子弹稍微离开坦克一点，避免立即与自身碰撞
    const float launchOffset = 25.0f;

    switch (currentTankDir) {
        case Direction::UP:
            bulletStartPos.y -= (tankHalfHeight + launchOffset);
            break;
        case Direction::DOWN:
            bulletStartPos.y += (tankHalfHeight + launchOffset);
            break;
        case Direction::LEFT:
            bulletStartPos.x -= (tankHalfWidth + launchOffset);
            break;
        case Direction::RIGHT:
            bulletStartPos.x += (tankHalfWidth + launchOffset);
            break;
    }

    // **重要提示关于子弹起始位置**：
    // 上述 bulletStartPos 的计算是基于子弹精灵的原点是其 *左上角*。
    // 如果你在 Bullet 构造函数中将子弹精灵的原点设置为了其中心 (bulletWidth/2, bulletHeight/2)，
    // 那么 bulletStartPos 应该直接计算为炮管口的中心点，例如：
    // case Direction::UP:
    //     bulletStartPos.x = m_position.x + tankWidth / 2.f;
    //     bulletStartPos.y = m_position.y - launchOffset; // 子弹的中心从坦克顶部略微向上
    //     break;
    // ...其他方向类似调整...
    // 这种方式通常更简单直观。我强烈建议在 Bullet 构造函数中设置精灵原点到中心。

    // 4. 定义子弹的其他属性
    int bulletDamage = 10;        // 示例伤害值
    float bulletSpeedValue = 200.f; // 示例速度值 (像素/秒)
    int bulletType = 0;           // 示例类型

    std::cout << "Tank::shoot() - Creating bullet:" << std::endl;
    std::cout << "  Texture: " << &bulletTexture << " (address of texture)"<< std::endl;
    std::cout << "  StartPos: (" << bulletStartPos.x << ", " << bulletStartPos.y << ")" << std::endl;
    std::cout << "  TankDirectionEnum: " << static_cast<int>(currentTankDir) << std::endl;
    std::cout << "  FlyDirectionVec: (" << flyVec.x << ", " << flyVec.y << ")" << std::endl;
    std::cout << "  Damage: " << bulletDamage << ", Speed: " << bulletSpeedValue << ", Type: " << bulletType << std::endl;

    // 5. 创建并返回 Bullet 对象
    //    确保这里的参数顺序和类型与你的 Bullet 构造函数完全一致！
    //    我上次建议的 Bullet 构造函数是：
    //    Bullet(const sf::Texture& texture, sf::Vector2f startPosition,
    //           Tank::Direction tankDirectionEnum, sf::Vector2f flyDirectionVec,
    //           int damage, float speed, int type);
    return std::make_unique<Bullet>(bulletTexture, bulletStartPos, currentTankDir, flyVec,
                                    bulletDamage, bulletSpeedValue, bulletType);
}

void Tank::takeDamage(int damageAmount) {
    if(m_Destroyed){
        return;
    }

    m_health -= damageAmount;
    std::cout << "Tank take damage: " << damageAmount << std::endl;
    if (m_health <= 0)
    {
        m_health = 0;
        m_Destroyed = true;
        std::cout << "Tank at (" << m_position.x << ", " << m_position.y << ") is destroyed!" << std::endl;
        //在这里可以触发一些视觉、音效，或者由Game类来处理后续逻辑
    }
}

void Tank::revive(sf::Vector2f position, Direction direction) {
    m_position = position;
    m_direction = direction;
    m_Destroyed = false;
    m_health = m_MaxHealth;
    m_sprite.setPosition(m_position);
    setDirection(direction);
    std::cout << "Tank at (" << m_position.x << ", " << m_position.y << ") is revived!" << std::endl;
}
