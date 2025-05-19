
#ifndef TANKS_MAP_H
#define TANKS_MAP_H

#include "heads.h"
#include <vector>
#include <map>
#include <random> // For std::mt19937

class Game; // Game的完整定义不需要，但Game&会用到

class Map {
private:
    std::vector<std::vector<int>> m_layout;
    std::vector<std::vector<int>> m_tileHealth;
    sf::Sprite m_tileSprite;
    int m_tileWidth;
    int m_tileHeight;
    int m_mapWidth;  // 地图宽度 (格子数)
    int m_mapHeight; // 地图高度 (格子数)

    int m_baseHealth;
    bool m_isBaseDestroyed;

    // void initMapLayout(); // 将被 generateLayout 取代或其逻辑并入
    void initializeTileHealth(); // 新增：辅助函数，根据布局初始化砖墙血量

public:
    static const int BRICK_INITIAL_HEALTH = 3;
    static const int BASE_INITIAL_HEALTH = 200; // 基地初始血量

    // 地图瓦片类型ID (与config.json和Game::getTexture中的键名对应)
    // 0: Grass (map_grass)
    // 1: Brick Wall (map_brick_wall, map_brick_wall_damaged1, map_brick_wall_damaged2)
    // 2: Steel Wall (map_steel_wall)
    // 3: Base (map_base)
    // 4: Water (map_water) - 新增
    // 5: Forest (map_forest) - 新增

    Map();
    bool loadDimensionsAndTextures(Game& game); // 修改：只加载尺寸和纹理信息，布局由generateLayout处理
    void generateLayout(int level, std::mt19937& rng, const Game& game); // 新增：根据关卡生成地图布局
    void draw(sf::RenderWindow &window, Game& game);
    bool isTileWalkable(int tileX, int tileY) const;
    int getTileWidth() const { return m_tileWidth; };
    int getTileHeight() const { return m_tileHeight; };

    sf::Vector2i getBaseTileCoordinate() const;
    int getMapWidth() const { return m_mapWidth; }
    int getMapHeight() const { return m_mapHeight; }
    int getTileType(int tileX, int tileY) const;

    int getTileHealth(int tileX, int tileY) const; // 获取砖墙血量
    void damageTile(int tileX, int tileY, int damage, Game& game); // 砖墙受损
    void damageBase(int damage); // 基地受损
    int getBaseHealth() const;
    bool isBaseDestroyed() const;
    void resetForNewLevel(); // 新增：重置地图状态（血量等），但不重新生成布局
};

#endif //TANKS_MAP_H