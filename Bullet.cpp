// Bullet.cpp
#include "Bullet.h" // 包含 Bullet.h 头文件
#include <iostream>   // 用于调试输出 (可选)

// 构造函数实现
Bullet::Bullet(const sf::Texture& texture,      // 子弹纹理 (已加载并设置为正确朝向)
               sf::Vector2f startPosition,      // 初始位置
               Direction tankDirectionEnum,     // 发射时坦克的方向
               sf::Vector2f flyDirectionVec,    // 标准化的飞行方向向量
               int damage,                      // 伤害值
               float speed,                     // 飞行速度
               int type                         /*, Tank* owner */) // 子弹类型 (可选的 owner)
        : m_position(startPosition),
          m_flyDirection(normalize(flyDirectionVec)), // 确保飞行向量被标准化
          m_tankDirection(tankDirectionEnum),
          m_speed(speed),
          m_damage(damage),
          m_type(type),
          m_isAlive(true)
// , m_owner(owner) // 如果启用了 owner
{
    // 设置精灵的纹理
    m_sprite.setTexture(texture);
    // 设置精灵的初始位置
    m_sprite.setPosition(m_position);

    // (可选) 设置精灵的原点到其中心，以便更精确地定位和旋转 (如果需要旋转的话)
    // 如果子弹纹理本身已经是最终朝向，则通常不需要旋转。
    // 原点设置对于碰撞检测的边界框 (getBounds) 如何计算也有影响。
    sf::FloatRect bounds = m_sprite.getLocalBounds();
    m_sprite.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    // 如果设置了原点，确保逻辑位置 m_position 代表的是子弹的中心点，
    // 并且在 Tank::shoot 中计算 bulletStartPos 时也考虑到这一点。
    // 如果 m_position 已经是期望的中心点，则在 setOrigin 后再次 setPosition 是正确的。
    m_sprite.setPosition(m_position);


    // 调试输出 (可选)
    // std::cout << "Bullet created. Type: " << m_type
    //           << ", Pos: (" << m_position.x << "," << m_position.y << ")"
    //           << ", FlyDir: (" << m_flyDirection.x << "," << m_flyDirection.y << ")"
    //           << ", Damage: " << m_damage << ", Speed: " << m_speed << std::endl;
}

// 更新子弹状态 (主要是移动)
void Bullet::update(sf::Time dt) {
    if (m_isAlive) {
        // 计算本帧的位移量
        float dtSeconds = dt.asSeconds();
        sf::Vector2f displacement = m_flyDirection * m_speed * dtSeconds;

        // 更新逻辑位置
        m_position += displacement;

        // 更新精灵的绘制位置
        m_sprite.setPosition(m_position);

        // (可选) 在这里可以添加子弹生命周期管理，例如飞出屏幕边界后 setIsAlive(false)
        // if (m_position.x < 0 || m_position.x > SCREEN_WIDTH || m_position.y < 0 || m_position.y > SCREEN_HEIGHT) {
        //     setIsAlive(false);
        // }
    }
}

// 绘制子弹
void Bullet::draw(sf::RenderWindow &window) const {
    if (m_isAlive) {
        window.draw(m_sprite);
    }
}

// 检查子弹是否存活
bool Bullet::isAlive() const {
    return m_isAlive;
}

// 设置子弹的存活状态
void Bullet::setIsAlive(bool alive) {
    m_isAlive = alive;
    if (!alive) {
        // (可选) 可以在子弹失效时进行一些清理或记录
        // std::cout << "Bullet at (" << m_position.x << ", " << m_position.y << ") is now inactive." << std::endl;
    }
}

// 获取子弹当前位置
sf::Vector2f Bullet::getPosition() const {
    return m_position;
}

// 获取子弹伤害值
int Bullet::getDamage() const {
    return m_damage;
}

// 获取子弹类型
int Bullet::getType() const {
    return m_type;
}

// 获取发射时坦克的方向
Direction Bullet::getTankDirection() const {
    return m_tankDirection;
}

// 获取子弹的全局边界框 (用于碰撞检测)
sf::FloatRect Bullet::getBounds() const {
    if (m_isAlive) {
        // getGlobalBounds() 返回的是精灵在世界坐标系中的边界框，
        // 它会考虑到精灵的变换 (位置, 旋转, 缩放, 原点)。
        return m_sprite.getGlobalBounds();
    }
    // 如果子弹不存活，返回一个空的边界框
    return sf::FloatRect();
}

// (可选) 获取子弹的发射者
// Tank* Bullet::getOwner() const {
//     return m_owner;
// }
