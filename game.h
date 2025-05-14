#ifndef TANKS_GAME_H
#define TANKS_GAME_H

#include <optional>
#include "heads.h"
#include "Map.h"
#include "tank.h"
#include "Bullet.h"

class PlayerTank;
class AITank;

enum class GameState{
    MainMenu,
    Playing1P,
    Playing2P,
    Settings,
    GameOver
};

class Game {

public:
    Game();
    ~Game();
    void init();
    void run();
    void end();

    void load_date();
    bool isWindowOpen() const {return window.isOpen();};
    const sf::Texture& getBulletTexture(Direction dir) const;

    void addBullet(std::unique_ptr<Bullet> bullet);
private:


    void Handling_events(sf::Time dt);
    void update(sf::Time dt);
    void render();
    int score;
    int life;
    sf::RenderWindow window;
    GameState state;
    Map m_map;
    std::vector<std::unique_ptr<Tank>> m_all_tanks;
    PlayerTank*  m_playerTankPtr;
    std::vector<std::unique_ptr<Bullet>> m_bullets;
    std::map <Direction, sf::Texture> m_bullet_textures;
    sf::Clock clock;
};


#endif //TANKS_GAME_H