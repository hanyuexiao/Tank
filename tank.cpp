// Tank.cpp
#include <iostream>
#include "tank.h"
#include "Bullet.h"
#include "Game.h" // Make sure Game.h is included for Game&

const int Tank::MAX_ARMOR;
// MODIFIED Constructor
Tank::Tank(sf::Vector2f startPosition, Direction startDirection, const std::string& tankType, Game& game,
           float speed, int frameWidth, int frameHeight, int iniHealth, int armor) :
        m_position(startPosition),
        m_direction(startDirection),
        m_tankType(tankType), // Store the tank type
        m_currentFrame(0),
        m_frameWidth(frameWidth),
        m_frameHeight(frameHeight),
        m_baseShootCooldown(sf::seconds(0.5f)), // Example value
        m_shootCooldown(m_baseShootCooldown),
        m_shootTimer(sf::Time::Zero),
        m_attackSpeedBuffDuration(sf::Time::Zero),
        m_movementSpeedBuffIncrease(0.0f),
        m_isAttackSpeedBuffActive(false),
        m_baseAttackPower(20), // Example base attack power
        m_currentAttackPower(m_baseAttackPower),
        m_attackBuffDuration(sf::Time::Zero),
        m_isAttackBuffActive(false),
        m_baseSpeed(speed),
        m_speed(m_baseSpeed),
        m_health(iniHealth),
        m_MaxHealth(iniHealth),
        m_Destroyed(false),
        m_armor(armor),
        m_isMovementSpeedBuffActive(false),
        m_movementSpeedBuffDuration(sf::Time::Zero),
        m_originalSpeed(speed),
        m_isInForest(false)
{
    // Textures are no longer loaded by a Tank::loadTextures() method.
    // Instead, we fetch the initial texture from the Game object.
    const auto& initialFrames = game.getTankTextures(m_tankType, m_direction);
    if (!initialFrames.empty()) {
        m_sprite.setTexture(initialFrames[0]); // Set initial texture (first frame)
    } else {
        std::cerr << "Tank Constructor Error: Initial texture could not be set for tank type '"
                  << m_tankType << "' and direction " << static_cast<int>(m_direction)
                  << ". Textures not found or empty in Game cache." << std::endl;
        // Consider loading a default/error texture or handling this error more gracefully
    }

    if (m_frameWidth > 0 && m_frameHeight > 0) {
        m_sprite.setOrigin(m_frameWidth / 2.f, m_frameHeight / 2.f);
    } else {
        std::cerr << "Tank Constructor Error: Frame width or height is zero for tank type '" << m_tankType << "'." << std::endl;
    }
    m_originalSpeed = m_baseSpeed;
    m_sprite.setPosition(m_position);
    std::cout << "Tank type '" << m_tankType << "' created." << std::endl;
}

// loadTextures() method is REMOVED from here. It's now handled by the Game class loading from JSON.

void Tank::draw(sf::RenderWindow& window) {
    window.draw(m_sprite);

    // Debug drawing for center point (optional)
    // sf::CircleShape centerDot(3.f);
    // centerDot.setFillColor(sf::Color::Red);
    // centerDot.setOrigin(3.f, 3.f);
    // centerDot.setPosition(m_position);
    // window.draw(centerDot);
}

// MODIFIED setDirection to use textures from Game
void Tank::setDirection(Direction dir, Game& game) {
    if (m_direction != dir) {
        m_direction = dir;
        m_currentFrame = 0; // Reset animation frame

        const auto& frames = game.getTankTextures(m_tankType, m_direction);
        if (!frames.empty()) {
            m_sprite.setTexture(frames[m_currentFrame]);
        } else {
            std::cerr << "Tank::setDirection Error: Texture not found for tank type '"
                      << m_tankType << "' and direction " << static_cast<int>(m_direction) << std::endl;
        }
    }
}

// MODIFIED update to use textures from Game for animation
void Tank::update(sf::Time dt, Game& game) {
    // 1. 更新动画 (这部分可以放在前面或后面，不影响速度计算的核心逻辑)
    const auto& frames = game.getTankTextures(m_tankType, m_direction);
    if (!frames.empty()) {
        int numFrames = frames.size();
        if (numFrames > 0) {
            m_currentFrame = (m_currentFrame + 1) % numFrames;
            m_sprite.setTexture(frames[m_currentFrame]);
        }
    }

    // 2. 更新射击计时器
    if (m_shootTimer < m_shootCooldown) {
        m_shootTimer += dt;
    }

    // 3. 处理各种 Buff 状态的倒计时和结束逻辑
    // 注意：对于速度buff，我们只处理它的结束。效果的应用会在速度计算部分统一处理。

    if (m_isAttackBuffActive) {
        m_attackBuffDuration -= dt;
        if (m_attackBuffDuration <= sf::Time::Zero) {
            m_currentAttackPower = m_baseAttackPower;
            m_isAttackBuffActive = false;
            m_attackBuffDuration = sf::Time::Zero;
            std::cout << "Attack buff for tank type '" << m_tankType << "' ended" << std::endl;
        }
    }

    if (m_isAttackSpeedBuffActive) {
        m_attackSpeedBuffDuration -= dt;
        if (m_attackSpeedBuffDuration <= sf::Time::Zero) {
            m_shootCooldown = m_baseShootCooldown;
            m_isAttackSpeedBuffActive = false;
            m_attackSpeedBuffDuration = sf::Time::Zero;
            std::cout << "Attack Speed buff for tank type '" << m_tankType << "' expired. Shoot cooldown reset to: " << m_shootCooldown.asSeconds() << "s" << std::endl;
        }
    }

    // --- 核心速度计算 ---
    // a. 从基础速度开始，应用地形效果
    const Map& map = game.getMap(); // 使用 const 引用获取地图
    int currentTileX = static_cast<int>(m_position.x / map.getTileWidth());
    int currentTileY = static_cast<int>(m_position.y / map.getTileHeight());
    int tileTypeOn = -1; // 默认无效地块
    if (map.getTileWidth() > 0 && map.getTileHeight() > 0) { // 确保瓦片尺寸有效
        tileTypeOn = map.getTileType(currentTileX, currentTileY);
    }


    float speedAfterTerrain = m_baseSpeed; // 以 m_baseSpeed 为准，而不是 m_originalSpeed
    // m_originalSpeed 主要是为了在 setSpeed 时同步 m_baseSpeed
    // 或者如果你希望地形减速是基于一个不可变的“出厂速度”，则用 m_originalSpeed
    // 但通常 m_baseSpeed 代表了当前“无临时buff/地形影响”下的速度

    if (tileTypeOn == 5) { // 假设 5 是雨林 Tile ID
        speedAfterTerrain *= FOREST_SLOWDOWN_FACTOR;
        m_isInForest = true;
    } else {
        m_isInForest = false;
    }

    // b. 应用移动速度 Buff (如果激活)
    if (m_isMovementSpeedBuffActive) {
        m_movementSpeedBuffDuration -= dt;
        if (m_movementSpeedBuffDuration <= sf::Time::Zero) { // Buff 结束
            m_isMovementSpeedBuffActive = false;
            m_movementSpeedBuffDuration = sf::Time::Zero;
            m_movementSpeedBuffIncrease = 0.0f; // Buff 结束，清除增加量
            m_speed = speedAfterTerrain; // 最终速度是地形调整后的速度
            std::cout << "Movement Speed buff for tank type '" << m_tankType << "' expired. Speed is now: " << m_speed << std::endl;
        } else { // Buff 仍然激活
            m_speed = speedAfterTerrain + m_movementSpeedBuffIncrease; // 在地形调整后的速度基础上增加buff量
        }
    } else { // 没有移动速度 buff 激活
        m_speed = speedAfterTerrain; // 最终速度就是地形调整后的速度
    }
    // --- 速度计算结束 ---
}

void Tank::move(sf::Vector2f targetPosition, const Map& map) {
    sf::Sprite prospectiveSprite = m_sprite; // Sprite uses currently set texture
    prospectiveSprite.setPosition(targetPosition);
    sf::FloatRect originalTankBoundsAtTarget = prospectiveSprite.getGlobalBounds();
    const float shrinkValue = 2.f;
    sf::FloatRect checkingBounds = originalTankBoundsAtTarget;

    if (checkingBounds.width > 2 * shrinkValue && checkingBounds.height > 2 * shrinkValue) {
        checkingBounds.left += shrinkValue;
        checkingBounds.top += shrinkValue;
        checkingBounds.width -= 2 * shrinkValue;
        checkingBounds.height -= 2 * shrinkValue;
    }

    int tileW = map.getTileWidth();
    int tileH = map.getTileHeight();

    if (tileW <= 0 || tileH <= 0) {
        std::cerr << "Error! Tank::move for type '" << m_tankType << "': Tile width or height is zero or negative. W: " << tileW << ", H: " << tileH << std::endl;
        return;
    }

    int startX = static_cast<int>(checkingBounds.left / tileW);
    int startY = static_cast<int>(checkingBounds.top / tileH);
    int endX = static_cast<int>((checkingBounds.left + checkingBounds.width - 0.001f) / tileW);
    int endY = static_cast<int>((checkingBounds.top + checkingBounds.height - 0.001f) / tileH);

    bool canMove = true;
    for (int y_tile = startY; y_tile <= endY; ++y_tile) {
        for (int x_tile = startX; x_tile <= endX; ++x_tile) {
            if (!map.isTileWalkable(x_tile, y_tile)) {
                canMove = false;
                break;
            }
        }
        if (!canMove) {
            break;
        }
    }

    if (canMove) {
        m_position = targetPosition;
        m_sprite.setPosition(m_position);
    }
}

// MODIFIED shoot to get bullet texture from Game cache using string key
void Tank::shoot(Game& gameInstance) { // ***修改返回类型为 void***
    if (m_shootTimer < m_shootCooldown) {
        // return nullptr; // 旧的返回
        return; // 直接返回，不射击
    }
    m_shootTimer = sf::Time::Zero;

    Direction currentTankDir = get_Direction();
    sf::Vector2f flyVec;
    std::string bulletTextureKey;

    // ... (switch case 设置 flyVec 和 bulletTextureKey 的逻辑不变) ...
    switch (currentTankDir) {
        case Direction::UP:    flyVec = sf::Vector2f(0.f, -1.f); bulletTextureKey = "bullet_up";    break;
        case Direction::DOWN:  flyVec = sf::Vector2f(0.f, 1.f);  bulletTextureKey = "bullet_down";  break;
        case Direction::LEFT:  flyVec = sf::Vector2f(-1.f, 0.f); bulletTextureKey = "bullet_left";  break;
        case Direction::RIGHT: flyVec = sf::Vector2f(1.f, 0.f);  bulletTextureKey = "bullet_right"; break;
        default:
            std::cerr << "Tank::shoot() for type '" << m_tankType << "' - Invalid tank direction!" << std::endl;
            // return nullptr; // 旧的返回
            return;
    }


    const sf::Texture& bulletTexture = gameInstance.getTexture(bulletTextureKey);
    if (bulletTexture.getSize().x == 0 || bulletTexture.getSize().y == 0) {
        std::cerr << "Tank::shoot() for type '" << m_tankType << "' - Failed to get bullet texture for key '" << bulletTextureKey << "' or texture is invalid." << std::endl;
        // return nullptr; // 旧的返回
        return;
    }

    // ... (计算 bulletStartPos 的逻辑不变) ...
    sf::Vector2f bulletStartPos = get_position();
    const float launchOffset = 25.0f;
    switch (currentTankDir) {
        case Direction::UP:    bulletStartPos.y -= (m_frameHeight / 2.f + launchOffset); break;
        case Direction::DOWN:  bulletStartPos.y += (m_frameHeight / 2.f + launchOffset); break;
        case Direction::LEFT:  bulletStartPos.x -= (m_frameWidth / 2.f + launchOffset);  break;
        case Direction::RIGHT: bulletStartPos.x += (m_frameWidth / 2.f + launchOffset);  break;
    }

    int bulletDamage = getCurrentAttackPower();
    float bulletSpeedValue = 200.f; // 子弹速度应该是一个可配置的或常量
    int bulletType = (m_tankType == "player") ? 1 : 2; // 简单示例：玩家子弹类型1，AI子弹类型2

    // ***从对象池获取并重置子弹***
    Bullet* bulletToShoot = gameInstance.getAvailableBullet();
    if (bulletToShoot) {
        bulletToShoot->reset(bulletTexture, bulletStartPos, currentTankDir, flyVec,
                             bulletDamage, bulletSpeedValue, bulletType);
        // bulletToShoot->setIsAlive(true); // reset 方法内部应该已经做了这个
        std::cout << "Tank type '" << m_tankType << "' shot a bullet from pool. TextureKey: " << bulletTextureKey << std::endl;
    } else {
        std::cout << "Tank type '" << m_tankType << "' failed to get a bullet from pool (pool might be full or error)." << std::endl;
    }
    // 不再返回 unique_ptr
    // return std::make_unique<Bullet>(bulletTexture, bulletStartPos, currentTankDir, flyVec,
    //                                 bulletDamage, bulletSpeedValue, bulletType);
}

void Tank::takeDamage(int damageAmount) {
    if (m_Destroyed) {
        return;
    }

    if (m_armor > 0) {
        m_armor--;
        std::cout << "Tank type '" << m_tankType << "' armor absorbed the damage. Armor left: " << m_armor << std::endl;
        return;
    }

    m_health -= damageAmount;
    std::cout << "Tank type '" << m_tankType << "' took damage: " << damageAmount << ". Health: " << m_health << std::endl;
    if (m_health <= 0) {
        m_health = 0;
        m_Destroyed = true;
        std::cout << "Tank type '" << m_tankType << "' at (" << m_position.x << ", " << m_position.y << ") is destroyed!" << std::endl;
    }
}

void Tank::revive(sf::Vector2f position, Direction direction, Game& game) { // Added Game& for texture consistency on revive
    m_position = position;
    m_direction = direction; // Set direction before fetching texture
    m_Destroyed = false;
    m_health = m_MaxHealth;
    m_armor = 0; // Reset armor, or to a default value
    m_shootTimer = sf::Time::Zero; // Reset shoot timer
    m_speed = m_baseSpeed;
    m_originalSpeed = m_baseSpeed;
    // Reset buffs
    m_isAttackBuffActive = false; m_currentAttackPower = m_baseAttackPower; m_attackBuffDuration = sf::Time::Zero;
    m_isAttackSpeedBuffActive = false; m_shootCooldown = m_baseShootCooldown; m_attackSpeedBuffDuration = sf::Time::Zero;
    m_isMovementSpeedBuffActive = false; m_speed = m_baseSpeed; m_movementSpeedBuffDuration = sf::Time::Zero;
    m_movementSpeedBuffIncrease = 0.0f; // ***新增重置***
    m_isInForest = false; // 确保雨林状态也重置


    m_sprite.setPosition(m_position);
    // Set texture for new direction using Game object
    const auto& frames = game.getTankTextures(m_tankType, m_direction);
    if (!frames.empty()) {
        m_currentFrame = 0; // Reset animation frame
        m_sprite.setTexture(frames[m_currentFrame]);
    } else {
        std::cerr << "Tank::revive Error: Texture not found for tank type '"
                  << m_tankType << "' and direction " << static_cast<int>(m_direction) << std::endl;
    }
    std::cout << "Tank type '" << m_tankType << "' at (" << m_position.x << ", " << m_position.y << ") is revived!" << std::endl;
}


void Tank::setArmor(int newArmor) {
    m_armor = std::max(0, std::min(newArmor, MAX_ARMOR)); // Ensure armor is within [0, MAX_ARMOR]
}

void Tank::activateAttackBuff(float multiplier, sf::Time duration) {
    // If already active, new buff might override or refresh. Simple refresh here.
    m_currentAttackPower = static_cast<int>(m_baseAttackPower * multiplier);
    m_attackBuffDuration = duration;
    m_isAttackBuffActive = true;
    std::cout << "Tank type '" << m_tankType << "' Attack buff activated! Current attack: " << m_currentAttackPower
              << " for " << duration.asSeconds() << "s" << std::endl;
}

int Tank::getCurrentAttackPower() const {
    return m_isAttackBuffActive ? m_currentAttackPower : m_baseAttackPower;
}

void Tank::activateAttackSpeedBuff(float cooldownMultiplier, sf::Time duration) {
    m_shootCooldown = m_baseShootCooldown * cooldownMultiplier;
    m_attackSpeedBuffDuration = duration;
    m_isAttackSpeedBuffActive = true;
    std::cout << "Tank type '" << m_tankType << "' Attack Speed buff activated! Shoot cooldown: " << m_shootCooldown.asSeconds()
              << "s for " << duration.asSeconds() << "s" << std::endl;
}

void Tank::setSpeed(float newSpeed) {
    m_baseSpeed = newSpeed;
    m_originalSpeed = newSpeed; // 确保 originalSpeed 也随基础速度更新
    if (!m_isMovementSpeedBuffActive && !m_isInForest) { // 如果没有buff且不在森林，当前速度也立即更新
        m_speed = newSpeed;
    }
    // 如果有buff或在森林，m_speed 会在下一帧的 update 中根据新的 m_baseSpeed 重新计算
}

void Tank::activateMovementSpeedBuff(float increaseAmount, sf::Time duration) {
    // Buff 不再直接修改 m_speed，而是记录增加量
    m_movementSpeedBuffIncrease = increaseAmount; // 保存增加量
    m_movementSpeedBuffDuration = duration;
    m_isMovementSpeedBuffActive = true;
    // m_speed 的实际更新会在 Tank::update 中基于地形和这个 buffIncrease 来计算
    std::cout << "Tank type '" << m_tankType << "' Movement Speed buff activated! Base speed will effectively increase by " << increaseAmount
              << " for " << duration.asSeconds() << "s (terrain effects will also apply)" << std::endl;
}

