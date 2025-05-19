// Map.cpp
// Created by admin on 2025/4/30.
//
#include "Map.h"
#include "Game.h" // 包含 Game.h 以便使用 Game& game
#include <iostream> // 用于错误输出

// --- 构造函数 ---
Map::Map() : m_tileWidth(0), m_tileHeight(0), m_mapWidth(0), m_mapHeight(0) {
    // 构造函数体可以为空，主要逻辑在 load() 中
}

// --- 公开的加载函数 ---
// MODIFIED: load 方法现在需要 Game 引用
bool Map::load(Game& game) {
    // 1. 初始化地图布局 (这部分逻辑不变)
    initMapLayout();

    // 2. 从 Game 对象获取图块尺寸信息
    //    我们假设 Game 对象已经从 config.json 加载了所有必要的纹理，
    //    并且我们可以通过一个已知的键 (例如 "map_grass") 来获取一个样本图块纹理以确定尺寸。
    //    你需要确保 "map_grass" (或你选择的任何键) 确实是你 JSON 配置中地图瓦片的一个键，
    //    并且 Game::getTexture() 能够返回它。
    const sf::Texture& sampleTexture = game.getTexture("map_grass"); // 假设 "map_grass" 是草地瓦片的键

    if (sampleTexture.getSize().x == 0 || sampleTexture.getSize().y == 0) {
        std::cerr << "Map::load() - Error: Could not get valid sample texture ('map_grass') from Game object to determine tile size." << std::endl;
        // 设置一个默认的回退值，或者让加载失败
        m_tileWidth = 50;  // 默认图块宽度
        m_tileHeight = 50; // 默认图块高度
        // return false; // 如果无法确定图块尺寸则加载失败
    } else {
        m_tileWidth = sampleTexture.getSize().x;
        m_tileHeight = sampleTexture.getSize().y;
//        std::cout << "Map tile size set from Game cache: " << m_tileWidth << "x" << m_tileHeight << std::endl;
    }

    // 3. (可选) 验证所有必需的地图纹理是否已在 Game 中加载
    //    这取决于你的错误处理策略。Game::getTexture() 在找不到纹理时应该已经有错误处理。
    //    例如，你可以检查几个关键纹理：
    if (game.getTexture("map_brick_wall").getSize().x == 0) { // 假设 "map_brick_wall" 是砖墙的键
        std::cerr << "Map::load() - Warning: Brick wall texture ('map_brick_wall') seems to be missing or invalid in Game cache." << std::endl;
    }
    if (game.getTexture("map_steel_wall").getSize().x == 0) { // 假设 "map_steel_wall" 是钢墙的键
        std::cerr << "Map::load() - Warning: Steel wall texture ('map_steel_wall') seems to be missing or invalid in Game cache." << std::endl;
    }
    if (game.getTexture("map_base").getSize().x == 0) { // 假设 "map_base" 是基地的键
        std::cerr << "Map::load() - Warning: Base texture ('map_base') seems to be missing or invalid in Game cache." << std::endl;
    }
    if(game.getTexture("map_grass").getSize().x == 0) { // 假设 "map_grass" 是草地的键
        std::cerr << "Map::load() - Warning: Grass texture ('map_grass') seems to be missing or invalid in Game cache." << std::endl;
    }
    if(game.getTexture("map_water").getSize().x == 0) { // 假设 "map_water" 是水的键
        std::cerr << "Map::load() - Warning: Water texture ('map_water') seems to be missing or invalid in Game cache." << std::endl;
    }
    if(game.getTexture("map_forest").getSize().x == 0) { // 假设 "map_forest" 是森林的键
        std::cerr << "Map::load() - Warning: Forest texture ('map_forest') seems to be missing or invalid in Game cache." << std::endl;
    }

    std::cout << "Map loaded successfully. Tile dimensions determined via Game object." << std::endl;
    return true; // 所有加载步骤完成
}


// --- 私有的地图布局初始化函数 ---
// 这个函数的逻辑保持不变，因为它定义了地图的结构 (哪些瓦片ID在哪里)
void Map::initMapLayout() {
    // 定义地图尺寸
    m_mapHeight = 15; // 15 行
    m_mapWidth = 30;  // 30 列

    // 初始化 m_layout 为全 0 (空地/草地)
    m_layout = std::vector<std::vector<int>>(m_mapHeight, std::vector<int>(m_mapWidth, 0));

    // 1. --- 添加完整的边界钢墙 (ID 2) ---
    for (int y = 0; y < m_mapHeight; ++y) {
        for (int x = 0; x < m_mapWidth; ++x) {
            if (y == 0 || y == m_mapHeight - 1 || x == 0 || x == m_mapWidth - 1) {
                m_layout[y][x] = 2; // 钢墙边界 (ID 2 对应 "map_steel_wall")
            }
        }
    }

    // 2. --- 放置基地 (ID 3) 及其保护砖墙 (ID 1) ---
    int basePosX = m_mapWidth / 2;

    m_layout[m_mapHeight - 1][basePosX] = 3;   // 基地核心 (ID 3 对应 "map_base")

    if (m_mapHeight > 1) {
        m_layout[m_mapHeight - 2][basePosX] = 1;   // 基地正上方砖 (ID 1 对应 "map_brick_wall")
        if (basePosX > 0) m_layout[m_mapHeight - 2][basePosX - 1] = 1;
        if (basePosX < m_mapWidth - 1) m_layout[m_mapHeight - 2][basePosX + 1] = 1;
        if (basePosX > 0) m_layout[m_mapHeight - 1][basePosX - 1] = 1;
        if (basePosX < m_mapWidth - 1) m_layout[m_mapHeight - 1][basePosX + 1] = 1;
    }
    if (m_mapHeight > 2) {
        m_layout[m_mapHeight - 3][basePosX] = 0; // 基地更上方留出空地 (ID 0 对应 "map_grass")
    }

    // 3. --- 设计内部迷宫结构 (砖墙 ID 1, 钢墙 ID 2) ---
    // (这里的迷宫设计逻辑与之前相同，只是注释了ID对应的纹理键名)
    // 行 2
    for (int x = 1; x < m_mapWidth - 1; ++x) {
        if (x > 2 && x < m_mapWidth - 3 && (x % 4 == 0 || x % 4 == 1) ) {
            m_layout[2][x] = 1; // 砖墙
        }
    }
    m_layout[2][m_mapWidth / 2 - 2] = 0; m_layout[2][m_mapWidth / 2 + 2] = 0;

    // 行 4
    for (int x = 1; x < m_mapWidth - 1; ++x) {
        if (x > 1 && x < m_mapWidth - 5 && (x % 5 == 0 || x % 5 == 1 || x % 5 == 2)) {
            m_layout[4][x] = 1; // 砖墙
        }
    }
    if (m_mapWidth > 6) { m_layout[4][3] = 0; m_layout[4][4] = 0; m_layout[4][m_mapWidth - 4] = 0; m_layout[4][m_mapWidth - 3] = 0;}

    // 行 6 (一些垂直的墙)
    int vWall1_X = m_mapWidth / 4; int vWall2_X = m_mapWidth * 3 / 4;
    for (int y = 1; y < 8; ++y) {
        if (y == 0 || y == m_mapHeight -1) continue;
        if (vWall1_X > 0 && vWall1_X < m_mapWidth -1) { m_layout[y][vWall1_X] = 1; m_layout[y][vWall1_X + 1] = 2; } // 砖, 钢
        if (vWall2_X > 0 && vWall2_X < m_mapWidth -1) { m_layout[y][vWall2_X - 1] = 2; m_layout[y][vWall2_X] = 1; } // 钢, 砖
    }
    if (vWall1_X > 0 && vWall1_X < m_mapWidth -1) m_layout[5][vWall1_X] = 0;
    if (vWall2_X > 0 && vWall2_X < m_mapWidth -1) m_layout[5][vWall2_X] = 0;

    // 行 8
    for (int x = 1; x < m_mapWidth - 1; ++x) { if (x >= 5 && x <= m_mapWidth - 6 && x % 3 == 0) { m_layout[8][x] = 1;}} // 砖墙
    if (m_mapWidth/2 > 0 && m_mapWidth/2 < m_mapWidth -1) m_layout[8][m_mapWidth/2] = 0;

    // 行 10
    for (int x = 1; x < m_mapWidth - 1; ++x) { if ( !(x > m_mapWidth/2 -3 && x < m_mapWidth/2+3) && (x > 3 && x < m_mapWidth -4 && (x % 6 <= 3) )) { m_layout[10][x] = 1;}} // 砖墙

    // 行 12 (靠近基地，m_mapHeight-3)
    if (m_mapHeight > 3) {
        for (int x = 1; x < m_mapWidth - 1; ++x) {
            if ( !(x >= basePosX - 2 && x <= basePosX + 2) && (x > 1 && x < m_mapWidth -2 && (x % 4 == 0 || x % 4 == 1) )) {
                m_layout[m_mapHeight - 3][x] = 1; // 砖墙
            }
        }
        if (basePosX - 3 > 0) m_layout[m_mapHeight - 3][basePosX - 3] = 2; // 钢墙
        if (basePosX + 3 < m_mapWidth -1) m_layout[m_mapHeight - 3][basePosX + 3] = 2; // 钢墙
    }

    // 一些内部的钢墙块 (ID 2)
    if (m_mapHeight > 4 && m_mapWidth > 7) { m_layout[3][5] = 2; m_layout[3][m_mapWidth - 6] = 2;}
    if (m_mapHeight > 8 && m_mapWidth > 10) { m_layout[7][8] = 2; m_layout[7][m_mapWidth - 9] = 2;}
    if (m_mapHeight > 12 && m_mapWidth > 5) { m_layout[11][3] = 2; m_layout[11][m_mapWidth - 4] = 2;}

    // 4. --- 确保玩家和AI出生点可通行且在边界内 ---
    if (m_mapHeight > 3 && m_mapWidth > 3) {
        m_layout[1][1] = 0; m_layout[1][2] = 0; m_layout[1][3] = 0;
        m_layout[2][1] = 0; m_layout[2][2] = 0;
        m_layout[3][1] = 0;
    }
    if (m_mapHeight > 3 && m_mapWidth > 3) {
        m_layout[1][m_mapWidth - 2] = 0; m_layout[1][m_mapWidth - 3] = 0; m_layout[1][m_mapWidth - 4] = 0;
        m_layout[2][m_mapWidth - 2] = 0; m_layout[2][m_mapWidth - 3] = 0;
        m_layout[3][m_mapWidth - 2] = 0;
    }

    std::cout << "Map layout initialized (" << m_mapHeight << "x" << m_mapWidth << ")." << std::endl;
}

// --- 绘制函数 ---
// MODIFIED: draw 方法现在需要 Game 引用
void Map::draw(sf::RenderWindow &window, Game& game) {
    if (m_layout.empty() || m_tileWidth == 0 || m_tileHeight == 0) {
        // 如果地图未初始化或图块尺寸未知，则不绘制
        return;
    }

    // 遍历地图布局
    for (int y = 0; y < m_mapHeight; ++y) {
        for (int x = 0; x < m_mapWidth; ++x) {
            int tileID = m_layout[y][x];
            std::string textureKey;

            // 将 tileID 映射到 Game 缓存中纹理的键名
            // 这个映射必须与你在 Game::loadConfig 中加载纹理时使用的键名一致
            switch(tileID) {
                case 0: textureKey = "map_grass"; break;       // 对应 JSON "grass"
                case 1: textureKey = "map_brick_wall"; break;  // 对应 JSON "brick_wall"
                case 2: textureKey = "map_steel_wall"; break;  // 对应 JSON "steel_wall"
                case 3: textureKey = "map_base"; break;        // 对应 JSON "base"
                case 4: textureKey = "map_water"; break;       // 对应 JSON "water"
                case 5: textureKey = "map_forest"; break;      // 对应 JSON "forest"
                default:
                    // std::cerr << "Map::draw() Warning: Unknown tile ID " << tileID << " at (" << x << "," << y << ")" << std::endl;
                    continue; // 跳过未知ID 或绘制一个默认的错误图块
            }

            const sf::Texture& texture = game.getTexture(textureKey); // 从 Game 获取纹理

            if (texture.getSize().x > 0 && texture.getSize().y > 0) { // 检查纹理是否有效
                m_tileSprite.setTexture(texture);
                m_tileSprite.setPosition(static_cast<float>(x * m_tileWidth), static_cast<float>(y * m_tileHeight));
                window.draw(m_tileSprite);
            } else {
                // 这个警告可能在游戏运行时非常频繁，如果纹理确实缺失
                // std::cerr << "Map::draw() Warning: Texture not found or invalid in Game cache for tile ID "
                //           << tileID << " (key: " << textureKey << ") at (" << x << "," << y << ")" << std::endl;
            }
        }
    }
}

bool Map::isTileWalkable(int tileX, int tileY) const {
    if(tileX < 0 || tileY < 0 || tileX >= m_mapWidth || tileY >= m_mapHeight) {
        return false; // 越界则不可行走
    }
    // 假设 ID 为 0 (草地) 的是可通行的
    // 其他 ID (1-砖, 2-钢, 3-基地) 是不可通行的
    int tileID = m_layout[tileY][tileX];
    bool walkable = (tileID == 0 || tileID == 5);
//    std::cout << "Tile (" << tileX << "," << tileY << ") is " << (walkable ? "walkable" : "not walkable") << std::endl;
    return walkable;
}

sf::Vector2i Map::getBaseTileCoordinate() const {
    if (m_mapWidth > 0 && m_mapHeight > 0) {
        // 根据 initMapLayout，基地核心 (ID 3) 在: m_layout[m_mapHeight - 1][m_mapWidth / 2]
        return sf::Vector2i(m_mapWidth / 2, m_mapHeight - 1);
    }
    std::cerr << "Map::getBaseTileCoordinate() called before map dimensions are set." << std::endl;
    return sf::Vector2i(-1, -1); // 表示无效或未找到
}

int Map::getTileType(int tileX, int tileY) const {
    if(tileX < 0 || tileY < 0 || tileX >= m_mapWidth || tileY >= m_mapHeight) {
        return -1; // 越界则返回无效类型
    }
    return m_layout[tileY][tileX];
}



// getTileWidth(), getTileHeight(), getMapWidth(), getMapHeight() 等 getter 方法保持不变
