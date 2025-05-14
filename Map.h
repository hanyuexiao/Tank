//
// Created by admin on 2025/4/30.
//
#include "heads.h"

#ifndef TANKS_MAP_H
#define TANKS_MAP_H


class Map {
private:
    std::vector<std::vector<int>> m_layout;         // 地图布局 (整数编码)
    std::map<int, sf::Texture> m_textures;          // 整数ID -> 纹理 映射
    sf::Sprite m_tileSprite;                        // 用于绘制的复用精灵
    int m_tileWidth;                                // 图块宽度 (像素)
    int m_tileHeight;                               // 图块高度 (像素)
    int m_mapWidth;                                 // 地图宽度 (格子数)
    int m_mapHeight;                                // 地图高度 (格子数)

    bool loadTextures();                            // 将加载纹理变为私有辅助函数
    void initMapLayout();                           // 将初始化布局变为私有辅助函数


public:
    Map();                                          // 构造函数
    bool load();                                    // 公开的加载函数，调用内部加载
    void draw(sf::RenderWindow &window);            // 绘制函数
    bool isTileWalkable(int tileX, int tileY) const;      // 判断某个格子是否可走
    int getTileWidth() const{return m_tileWidth;};                     // 获取图块宽度
    int getTileHeight() const{return m_tileHeight;};                    // 获取图块高度

    sf::Vector2i getBaseTileCoordinate() const;
    int getMapWidth() const { return m_mapWidth; } // 确保有这些getter
    int getMapHeight() const { return m_mapHeight; }
    // 未来可能需要的函数:
    // bool isSolid(int tileX, int tileY);          // 检查某格子是否为固体墙
    // int getTileType(int tileX, int tileY);       // 获取某格子的类型
    // void destroyTile(int tileX, int tileY);      // 销毁某个可破坏的图块
};

#endif //TANKS_MAP_H

