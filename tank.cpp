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
        m_movementSpeedBuffDuration(sf::Time::Zero)
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
    // Animation update logic‘
//    if (this->getTankType() == "player") { // 确保 getTankType() 返回 "player"
//        std::cout << "PlayerTank::update() called. DeltaTime: " << dt.asSeconds()
//                  << "s. Current shootTimer: " << m_shootTimer.asSeconds()
//                  << "s. shootCooldown: " << m_shootCooldown.asSeconds() << "s." << std::endl;
//    }
    const auto& frames = game.getTankTextures(m_tankType, m_direction);
    if (!frames.empty()) {
        // Assuming 2 frames for animation as in the original loadTextures logic.
        // If the number of frames can vary, use frames.size()
        int numFrames = frames.size();
        if (numFrames > 0) { // Ensure there are frames before trying to animate
            m_currentFrame = (m_currentFrame + 1) % numFrames;
            m_sprite.setTexture(frames[m_currentFrame]);
        }
    } else {
        // This warning might be too verbose if textures are consistently missing.
        // std::cerr << "Tank::update Warning: Animation frames not found for tank type '"
        //           << m_tankType << "' and direction " << static_cast<int>(m_direction) << std::endl;
    }


    if (m_shootTimer < m_shootCooldown) {
        m_shootTimer += dt;
    }

    // Buff timers
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

    if (m_isMovementSpeedBuffActive) {
        m_movementSpeedBuffDuration -= dt;
        if (m_movementSpeedBuffDuration <= sf::Time::Zero) {
            m_speed = m_baseSpeed;
            m_isMovementSpeedBuffActive = false;
            m_movementSpeedBuffDuration = sf::Time::Zero;
            std::cout << "Movement Speed buff for tank type '" << m_tankType << "' expired. Speed reset to: " << m_speed << std::endl;
        }
    }
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
std::unique_ptr<Bullet> Tank::shoot(Game& gameInstance) {
    if (m_shootTimer < m_shootCooldown) {
        // Optional: More specific logging
        // std::cout << "Tank type '" << m_tankType << "' shoot on cooldown. Timer:"
        //           << m_shootTimer.asSeconds() << "s/"
        //           << m_shootCooldown.asSeconds() << "s" << std::endl;
        return nullptr;
    }
    m_shootTimer = sf::Time::Zero;

    Direction currentTankDir = get_Direction();
    sf::Vector2f flyVec;
    std::string bulletTextureKey; // Key to fetch texture from Game cache

    switch (currentTankDir) {
        case Direction::UP:    flyVec = sf::Vector2f(0.f, -1.f); bulletTextureKey = "bullet_up";    break;
        case Direction::DOWN:  flyVec = sf::Vector2f(0.f, 1.f);  bulletTextureKey = "bullet_down";  break;
        case Direction::LEFT:  flyVec = sf::Vector2f(-1.f, 0.f); bulletTextureKey = "bullet_left";  break;
        case Direction::RIGHT: flyVec = sf::Vector2f(1.f, 0.f);  bulletTextureKey = "bullet_right"; break;
        default:
            std::cerr << "Tank::shoot() for type '" << m_tankType << "' - Invalid tank direction!" << std::endl;
            return nullptr;
    }

    const sf::Texture& bulletTexture = gameInstance.getTexture(bulletTextureKey);
    if (bulletTexture.getSize().x == 0 || bulletTexture.getSize().y == 0) { // Check if texture is valid
        std::cerr << "Tank::shoot() for type '" << m_tankType << "' - Failed to get bullet texture for key '" << bulletTextureKey << "' or texture is invalid." << std::endl;
        return nullptr;
    }

    sf::Vector2u bulletTextureSize = bulletTexture.getSize(); // Should be valid now
    float bulletWidth = static_cast<float>(bulletTextureSize.x);
    float bulletHeight = static_cast<float>(bulletTextureSize.y);

    sf::Vector2f bulletStartPos = get_position();
    // Using m_frameWidth and m_frameHeight (tank's frame dimensions) for positioning the bullet relative to the tank's sprite center.
    // This assumes the bullet should originate from the center of the tank sprite's edge.
    // A more precise way would be to define炮管口 offsets in the JSON or per tank type.
    const float launchOffset = 25.0f; // How far from the tank's edge the bullet spawns

    switch (currentTankDir) {
        case Direction::UP:    bulletStartPos.y -= (m_frameHeight / 2.f + launchOffset); break;
        case Direction::DOWN:  bulletStartPos.y += (m_frameHeight / 2.f + launchOffset); break;
        case Direction::LEFT:  bulletStartPos.x -= (m_frameWidth / 2.f + launchOffset);  break;
        case Direction::RIGHT: bulletStartPos.x += (m_frameWidth / 2.f + launchOffset);  break;
    }

    int bulletDamage = getCurrentAttackPower();
    float bulletSpeedValue = 200.f;
    int bulletType = 0; // Example type

    // std::cout << "Tank type '" << m_tankType << "' creating bullet. TextureKey: " << bulletTextureKey
    //           << ", StartPos: (" << bulletStartPos.x << ", " << bulletStartPos.y << ")" << std::endl;

    return std::make_unique<Bullet>(bulletTexture, bulletStartPos, currentTankDir, flyVec,
                                    bulletDamage, bulletSpeedValue, bulletType);
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
    // Reset buffs
    m_isAttackBuffActive = false; m_currentAttackPower = m_baseAttackPower; m_attackBuffDuration = sf::Time::Zero;
    m_isAttackSpeedBuffActive = false; m_shootCooldown = m_baseShootCooldown; m_attackSpeedBuffDuration = sf::Time::Zero;
    m_isMovementSpeedBuffActive = false; m_speed = m_baseSpeed; m_movementSpeedBuffDuration = sf::Time::Zero;


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
    // This might be a bit tricky if also managed by a buff.
    // If this is a permanent change, m_baseSpeed should also change.
    // If it's temporary, it should be handled by the buff system.
    // For now, assuming it's a direct speed set, potentially overriding buffs.
    m_speed = newSpeed;
    if (!m_isMovementSpeedBuffActive) { // If no buff is active, also update base speed
        m_baseSpeed = newSpeed;
    }
}

void Tank::activateMovementSpeedBuff(float increaseAmount, sf::Time duration) {
    // Buff applies to the base speed
    m_speed = m_baseSpeed + increaseAmount;
    m_movementSpeedBuffDuration = duration;
    m_isMovementSpeedBuffActive = true;
    std::cout << "Tank type '" << m_tankType << "' Movement Speed buff activated! Current speed: " << m_speed
              << " for " << duration.asSeconds() << "s" << std::endl;
}

