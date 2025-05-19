// Map.h
// Created by admin on 2025/4/30.
//
#ifndef TANKS_MAP_H
#define TANKS_MAP_H

#include "heads.h" // 包含项目通用的头文件 (SFML, iostream, vector, map, memory, etc.)
#include <vector>   // 确保 std::vector 被包含
#include <map>      // 确保 std::map 被包含 (虽然 m_textures 移除了，但以防万一其他地方可能间接用到)

// 前向声明 Game 类，因为 Map 的方法现在需要 Game 的引用
class Game;

class Map {
private:
    std::vector<std::vector<int>> m_layout;         // 地图布局 (整数编码)
    // std::map<int, sf::Texture> m_textures;       // REMOVED: 纹理现在由 Game 类管理
    sf::Sprite m_tileSprite;                        // 用于绘制的复用精灵
    int m_tileWidth;                                // 图块宽度 (像素)
    int m_tileHeight;                               // 图块高度 (像素)
    int m_mapWidth;                                 // 地图宽度 (格子数)
    int m_mapHeight;                                // 地图高度 (格子数)

    // bool loadTextures();                         // REMOVED: 纹理加载逻辑移至 Game 类
    void initMapLayout();                           // 将初始化布局变为私有辅助函数

public:
    Map();                                          // 构造函数
    // MODIFIED: load 方法现在需要 Game 引用来获取图块尺寸等信息
    bool load(Game& game);
    // MODIFIED: draw 方法现在需要 Game 引用来获取纹理
    void draw(sf::RenderWindow &window, Game& game);
    bool isTileWalkable(int tileX, int tileY) const;      // 判断某个格子是否可走
    int getTileWidth() const { return m_tileWidth; };     // 获取图块宽度
    int getTileHeight() const { return m_tileHeight; };   // 获取图块高度

    sf::Vector2i getBaseTileCoordinate() const;
    int getMapWidth() const { return m_mapWidth; }        // 确保有这些getter
    int getMapHeight() const { return m_mapHeight; }
    int getTileType(int tileX, int tileY) const;       // 获取某格子的类型

    // 未来可能需要的函数:
    // bool isSolid(int tileX, int tileY);          // 检查某格子是否为固体墙
    // void destroyTile(int tileX, int tileY);      // 销毁某个可破坏的图块
};

#endif //TANKS_MAP_H
