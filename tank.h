#ifndef TANK_H
#define TANK_H

#include "heads.h"
#include "Map.h"
#include <vector>
#include <string>
#include "common.h"
class Bullet;
class Game;



class Tank {

protected:
    std::vector<sf::Texture> m_textures; // 存储不同方向的纹理
    sf::Sprite m_sprite;
    sf::Vector2f m_position;
    Direction m_direction;
    int m_currentFrame;
    int m_frameWidth;
    int m_frameHeight;
    sf::Time m_shootCooldown;
    sf::Time m_shootTimer;
    float m_speed;
    void loadTextures(); // 加载纹理

public:

    virtual ~Tank()  = default;

    void draw(sf::RenderWindow& window);

    virtual void update(sf::Time dt);

    void setDirection(Direction dir);

    void move(sf::Vector2f targetPosition,const Map& map);

    sf::Vector2f get_position() const {return m_position;}

    float getSpeed() const {return m_speed;}
    std::unique_ptr<Bullet> shoot(Game& gameInstance);

    Direction get_Direction(){return m_direction;}

    sf::Vector2f get_position(){return m_position;}

    int get_TileWight() const{return m_frameWidth;};

    int get_TileHeight() const{return m_frameHeight;};

    sf::FloatRect getBounds() const { return m_sprite.getGlobalBounds(); }

    bool canShoot() const { return m_shootTimer >= m_shootCooldown; }
    void resetShootTimer() { m_shootTimer = sf::Time::Zero; }

    Tank(sf::Vector2f startPosition, Direction direction,float speed = 100.f,int frameWidth=50, int frameHeight=50);

};

#endif // TANK_H