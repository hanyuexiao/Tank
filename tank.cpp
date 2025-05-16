#include <iostream>
#include "tank.h"
#include "Bullet.h"
#include "Game.h"

Tank::Tank(sf::Vector2f startPosition, Direction startDirection,float speed,int frameWidth, int frameHeight,int iniHealth,int m_armor) :
        m_position(startPosition),
        m_direction(startDirection),
        m_currentFrame(0),
        m_frameWidth(frameWidth),         // 使用传入的参数
        m_frameHeight(frameHeight),       // 使用传入的参数
        m_baseShootCooldown(sf::seconds(0.5f)),
        m_shootCooldown(m_baseShootCooldown),
        m_shootTimer(sf::Time::Zero),
        m_attackSpeedBuffDuration(sf::Time::Zero),
        m_isAttackSpeedBuffActive(false),
        m_isAttackBuffActive(false),
        m_baseSpeed(speed),
        m_speed(m_baseSpeed),// *** 确保 m_shootTimer 在这里初始化 ***
        m_health(iniHealth),
        m_MaxHealth(iniHealth),
        m_Destroyed(false),
        m_armor(m_armor),
        m_isMovementSpeedBuffActive(false),
        m_movementSpeedBuffDuration(sf::Time::Zero)
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

    if(m_isAttackBuffActive){
        m_attackBuffDuration -= dt;
        if(m_attackBuffDuration <= sf::Time::Zero){
            m_currentAttackPower = m_baseAttackPower;
            m_isAttackBuffActive = false;
            m_attackBuffDuration = sf::Time::Zero;
            std::cout << "Attack buff duration ended" << std::endl;
        }
    }

    if (m_isAttackSpeedBuffActive) {
        m_attackSpeedBuffDuration -= dt; // 使用攻速buff的计时器 m_shootCooldownBuffDuration
        if (m_attackSpeedBuffDuration <= sf::Time::Zero) {
            m_shootCooldown = m_baseShootCooldown; // 恢复基础射击冷却
            m_isAttackSpeedBuffActive = false;
            m_attackSpeedBuffDuration = sf::Time::Zero;
            std::cout << "Attack Speed buff expired. Shoot cooldown reset to: " << m_shootCooldown.asSeconds() << "s" << std::endl;
        }
    }

    if (m_isMovementSpeedBuffActive) {
        m_movementSpeedBuffDuration -= dt;
        if (m_movementSpeedBuffDuration <= sf::Time::Zero) {
            m_speed = m_baseSpeed; // 恢复基础移动速度
            m_isMovementSpeedBuffActive = false;
            m_movementSpeedBuffDuration = sf::Time::Zero;
            std::cout << "Movement Speed buff expired. Speed reset to: " << m_speed << "s" << std::endl;
        }
    }
}

void Tank::move(sf::Vector2f targetPosition, const Map& map) {
    // --- 步骤 1: 获取坦克在 *目标位置* 的原始边界框 ---
    sf::Sprite prospectiveSprite = m_sprite;
    prospectiveSprite.setPosition(targetPosition);
    sf::FloatRect originalTankBoundsAtTarget = prospectiveSprite.getGlobalBounds();

    // --- 调整：略微缩小用于碰撞检测的边界框 ---
    // 这有助于坦克更好地适应与其视觉大小相同的格子间隙，
    // 以补偿浮点数精度和精确边缘对齐带来的问题。
    // 'shrinkValue' 的值决定了检测框在每个方向上缩小的像素值。
    // 例如，0.5f 意味着检测框的宽度和高度各减少1个像素。
    const float shrinkValue = 2.f; // 可以尝试 0.1f 到 1.0f 之间的值
    sf::FloatRect checkingBounds = originalTankBoundsAtTarget;

    // 确保缩小后的宽高仍然为正
    if (checkingBounds.width > 2 * shrinkValue && checkingBounds.height > 2 * shrinkValue) {
        checkingBounds.left += shrinkValue;
        checkingBounds.top += shrinkValue;
        checkingBounds.width -= 2 * shrinkValue;  // 总宽度减少 2 * shrinkValue
        checkingBounds.height -= 2 * shrinkValue; // 总高度减少 2 * shrinkValue
    }
    // --- 调整结束 ---


    // --- 调试打印信息可以取消注释来观察边界框的变化 ---
    // std::cout << "Tank::move 尝试移动至: (" << targetPosition.x << ", " << targetPosition.y << ")" << std::endl;
    // std::cout << "  原始边界: L" << originalTankBoundsAtTarget.left << " T" << originalTankBoundsAtTarget.top << " W" << originalTankBoundsAtTarget.width << " H" << originalTankBoundsAtTarget.height << std::endl;
    // std::cout << "  检测边界: L" << checkingBounds.left << " T" << checkingBounds.top << " W" << checkingBounds.width << " H" << checkingBounds.height << std::endl;


    // --- 步骤 2: 计算调整后的检测边界框 (checkingBounds) 覆盖的地图瓦片范围 ---
    int tileW = map.getTileWidth();
    int tileH = map.getTileHeight();

    if (tileW <= 0 || tileH <= 0) {
        std::cerr << "错误! Tank::move 中的瓦片宽度或高度为零或负数。宽: " << tileW << ", 高: " << tileH << std::endl;
        return;
    }

    // 使用调整后的 checkingBounds 进行计算
    int startX = static_cast<int>(checkingBounds.left / tileW);
    int startY = static_cast<int>(checkingBounds.top / tileH);
    int endX = static_cast<int>((checkingBounds.left + checkingBounds.width - 0.001f) / tileW);
    int endY = static_cast<int>((checkingBounds.top + checkingBounds.height - 0.001f) / tileH);

    // ... (后续的碰撞检测循环逻辑保持不变, 使用 startX, startY, endX, endY) ...

    bool canMove = true;
    // 添加一个检查，确保计算出的瓦片范围是有效的
    if (endX < startX || endY < startY) {
        // 如果 shrinkValue 相对于坦克尺寸过大，或者坦克在地图检测的有效区域之外，可能会发生这种情况。
        // 根据地图设计，这可能意味着 canMove = false 或只是一个警告。
        // 为安全起见，如果范围奇怪，则视为无法移动。
        // 但如果 map.isTileWalkable 能够优雅地处理越界的 tileX/tileY（返回false），
        // 并且下面的循环是稳健的，那么这个显式检查可能不是严格必需的。
        // std::cout << "  警告: Tank::move 缩小后的检测瓦片范围无效 (end < start)。" << std::endl;
    }

    for (int y_tile = startY; y_tile <= endY; ++y_tile) {
        for (int x_tile = startX; x_tile <= endX; ++x_tile) {
            bool walkable = map.isTileWalkable(x_tile, y_tile);
            // std::cout << "    检测瓦片 (" << x_tile << "," << y_tile << "): " << (walkable ? "可通行" : "阻塞") << std::endl;
            if (!walkable) {
                canMove = false;
                break;
            }
        }
        if (!canMove) {
            break;
        }
    }

    // --- 步骤 4: 如果所有检查的瓦片都可通行，则更新坦克位置 ---
    if (canMove) {
        m_position = targetPosition;       // 更新坦克的逻辑位置
        m_sprite.setPosition(m_position);  // 更新精灵的绘制位置
    } else {
        // std::cout << "  Tank::move 被阻塞。" << std::endl;
    }
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
    int bulletDamage = getCurrentAttackPower();      // 示例伤害值
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

    if(m_armor > 0){
        m_armor--;
        std::cout << "Tank armor absorbed the damage. Armor left: " << m_armor << std::endl;
        return ;
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

void Tank::setArmor(int newArmor) {
    if(newArmor < 0){
        m_armor = 0;
    }else if (newArmor > 1){
        m_armor = 1;
    }else{
        m_armor = newArmor;
    }
}

void Tank::activateAttackBuff(float multiplier, sf::Time duration) {
    if (!m_isAttackBuffActive) {
        // 首次激活buff，或者之前的buff已过期
        // 确保 m_baseAttackPower 是在构造函数中正确初始化的，并且不会在这里被 m_currentAttackPower 覆盖
        m_currentAttackPower = static_cast<int>(m_baseAttackPower * multiplier);
        std::cout << "Attack buff activated! Base: " << m_baseAttackPower
                  << ", Multiplier: " << multiplier
                  << ", Current attack: " << m_currentAttackPower << std::endl;
    } else {
        // Buff 已经在激活状态，我们只刷新持续时间
        // 通常，如果再次拾取同类buff，我们不会改变倍率，除非新buff的倍率更高
        // 这里我们简单地刷新持续时间，攻击力倍数由第一次激活决定，或由传入的 multiplier 决定（如果希望每次都重新计算）
        // 为了确保攻击力是基于最新的基础攻击力和传入的倍率，我们还是重新计算一下：
        m_currentAttackPower = static_cast<int>(m_baseAttackPower * multiplier);
        std::cout << "Attack buff refreshed! Base: " << m_baseAttackPower
                  << ", Multiplier: " << multiplier
                  << ", Current attack: " << m_currentAttackPower << std::endl;
    }

    m_attackBuffDuration = duration; // 无论是首次激活还是刷新，都将持续时间设置为新的duration
    m_isAttackBuffActive = true;     // 确保buff状态为激活

    std::cout << "Attack buff duration set to " << m_attackBuffDuration.asSeconds() << "s" << std::endl;
}

int Tank::getCurrentAttackPower() const {
    return m_currentAttackPower;
}

void Tank::activateAttackSpeedBuff(float newCooldownMultiplier, sf::Time duration) {
    // 如果已激活，则刷新时间；否则，应用新的冷却时间
    m_shootCooldown = m_baseShootCooldown * newCooldownMultiplier; // 注意是乘法，因为我们传入的是冷却时间的倍率

    m_attackSpeedBuffDuration = duration; // 使用新的变量名 m_shootCooldownBuffDuration 会更好
    m_isAttackSpeedBuffActive = true;

    std::cout << "Attack Speed buff activated! Current shoot cooldown: " << m_shootCooldown.asSeconds()
              << "s (Base: " << m_baseShootCooldown.asSeconds() << "s) for "
              << duration.asSeconds() << "s" << std::endl;
}

void Tank::setSpeed(float newSpeed) {
    m_speed = newSpeed;
}

// tank.cpp
void Tank::activateMovementSpeedBuff(float increaseAmount, sf::Time duration) {
    if (!m_isMovementSpeedBuffActive) {
        // 首次激活，应用增量
        // m_baseSpeed 已经在构造时设置好了
    }
    // 无论是否已激活，都基于最新的 m_baseSpeed 重新计算，并刷新持续时间
    m_speed = m_baseSpeed + increaseAmount;

    m_movementSpeedBuffDuration = duration;
    m_isMovementSpeedBuffActive = true;

    std::cout << "Movement Speed buff activated! Current speed: " << m_speed
              << " (Base: " << m_baseSpeed << ") for "
              << duration.asSeconds() << "s" << std::endl;
}