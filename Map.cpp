// Map.cpp
// Created by admin on 2025/4/30.
//
#include "Map.h"
#include <iostream> // 用于错误输出

// --- 构造函数 ---
// 可以为空，或者设置一些默认值
Map::Map() : m_tileWidth(0), m_tileHeight(0), m_mapWidth(0), m_mapHeight(0) {
    // 构造函数体可以为空，将加载逻辑放到 load() 中
}

// --- 公开的加载函数 ---
bool Map::load() {
    // 1. 初始化地图布局
    initMapLayout(); // 调用私有方法初始化布局

    // 2. 加载纹理
    if (!loadTextures()) { // 调用私有方法加载纹理
        return false; // 如果纹理加载失败，则整体加载失败
    }

    // 可以在这里验证一下纹理尺寸是否一致，并设置 m_tileWidth/Height
    if (!m_textures.empty()) {
        // 假设所有图块大小一样，用第一个加载的纹理尺寸
        // 注意: map 迭代器的顺序不保证，用 find(1) 或其他确定存在的ID更好
        auto it = m_textures.find(1); // 假设 ID 1 (砖墙) 肯定存在
        if (it != m_textures.end()) {
            m_tileWidth = it->second.getSize().x;
            m_tileHeight = it->second.getSize().y;
            std::cout << "Tile size set to: " << m_tileWidth << "x" << m_tileHeight << std::endl;
        } else {
            std::cerr << "Warning: Could not determine tile size from texture ID 1." << std::endl;
            // 可以设置一个默认值或者返回错误
            m_tileWidth = 50; // 设置默认值
            m_tileHeight = 50;
        }
    } else {
        std::cerr << "Warning: No textures loaded." << std::endl;
        return false; // 没有纹理也算加载失败
    }


    return true; // 所有加载步骤完成
}


// --- 私有的纹理加载函数 ---
bool Map::loadTextures() {
    sf::Texture tempTexture;
    //加载草地 (ID 0)
    if(!tempTexture.loadFromFile("C:\\Users\\admin\\CLionProjects\\Tanks\\Wall\\block.png")) { // 假设图片在 images 目录下
        std::cerr << "Error loading grass.png" << std::endl;
        return false;
    }
    m_textures[0] = tempTexture;

    // 加载砖墙 (ID 1)
    if (!tempTexture.loadFromFile("C:\\Users\\admin\\CLionProjects\\Tanks\\Wall\\wallblock.png")) { // 假设图片在 images 目录下
        std::cerr << "Error loading brick.png" << std::endl;
        return false;
    }
    m_textures[1] = tempTexture; // 存入 map

    // 加载钢墙 (ID 2)
    if (!tempTexture.loadFromFile("C:\\Users\\admin\\CLionProjects\\Tanks\\Wall\\steelwall2.png")) {
        std::cerr << "Error loading steel.png" << std::endl;
        return false;
    }
    m_textures[2] = tempTexture;

    // 加载基地 (ID 3)
    if (!tempTexture.loadFromFile("C:\\Users\\admin\\CLionProjects\\Tanks\\Wall\\base3.png")) {
        std::cerr << "Error loading base.png" << std::endl;
        return false;
    }
    m_textures[3] = tempTexture;

    // 加载其他你需要的纹理... (比如树林、水面等)

    std::cout << "Textures loaded successfully." << std::endl;
    return true;
}

sf::Vector2i Map::getBaseTileCoordinate() const {
    // 根据你的 initMapLayout，大本营核心 (ID 3) 在:
    // m_layout[m_mapHeight - 1][m_mapWidth / 2]
    if (m_mapWidth > 0 && m_mapHeight > 0) { // 确保地图已初始化
        return sf::Vector2i(m_mapWidth / 2, m_mapHeight - 1);
    }
    std::cerr << "Map::getBaseTileCoordinate() called before map dimensions are set." << std::endl;
    return sf::Vector2i(-1, -1); // 表示无效或未找到
}
// --- 私有的地图布局初始化函数 ---
void Map::initMapLayout() {
    // 定义地图尺寸
    m_mapHeight = 15; // 例如 15 行
    m_mapWidth = 30;  // 例如 30 列

    // 初始化 m_layout 为全 0 (空地)
    m_layout = std::vector<std::vector<int>>(m_mapHeight, std::vector<int>(m_mapWidth, 0));

    // --- 在这里填充你的固定地图布局 ---
    // 例子：添加边界钢墙
    for(int y = 0; y < m_mapHeight; ++y) {
        if (y == 0 || y == m_mapHeight - 1) { // 上下边界
            for (int x = 0; x < m_mapWidth; ++x) m_layout[y][x] = 2; // 钢墙
        } else {
            m_layout[y][0] = 2; // 左边界
            m_layout[y][m_mapWidth - 1] = 2; // 右边界
        }
    }

    // 例子：放置基地 (ID 3) 在底部中间
    int basePosX = m_mapWidth / 2;
    m_layout[m_mapHeight - 2][basePosX -1] = 1; // 基地左边砖
    m_layout[m_mapHeight - 2][basePosX] = 1;   // 基地上方砖
    m_layout[m_mapHeight - 2][basePosX +1] = 1; // 基地右边砖
    m_layout[m_mapHeight - 1][basePosX -1] = 1; // 基地左下方砖 (边界已经是钢墙了)
    m_layout[m_mapHeight - 1][basePosX] = 3;   // 基地 E
    m_layout[m_mapHeight - 1][basePosX +1] = 1; // 基地右下方砖 (边界已经是钢墙了)


    // 例子：添加一些内部障碍物
    for (int i = 5; i < 10; ++i) {
        m_layout[i][m_mapWidth / 4] = 1; // 一列砖墙
        m_layout[i][m_mapWidth * 3 / 4] = 2; // 一列钢墙
    }

    std::cout << "Map layout initialized (" << m_mapHeight << "x" << m_mapWidth << ")." << std::endl;
}


// --- 绘制函数 ---
void Map::draw(sf::RenderWindow &window) {
    if (m_layout.empty() || m_textures.empty() || m_tileWidth == 0 || m_tileHeight == 0) {
        // 如果地图未初始化或纹理未加载或尺寸未知，则不绘制
        return;
    }

    // 遍历地图布局
    for (int y = 0; y < m_mapHeight; ++y) {
        for (int x = 0; x < m_mapWidth; ++x) {
            int tileID = m_layout[y][x];

            // 查找 ID 对应的纹理
            auto it = m_textures.find(tileID);
            if (it != m_textures.end()) {
                // 找到了纹理
                sf::Texture& texture = it->second; // 获取纹理的引用

                // 设置复用精灵的纹理
                m_tileSprite.setTexture(texture);

                // 计算像素位置
                float pixelX = static_cast<float>(x * m_tileWidth);
                float pixelY = static_cast<float>(y * m_tileHeight);

                // 设置精灵位置
                m_tileSprite.setPosition(pixelX, pixelY);

                // 绘制精灵
                window.draw(m_tileSprite);
            } else {
                // 警告：地图数据中的 tileID 在纹理 map 中找不到对应的纹理
                // 这种情况理论上不应发生，如果发生了说明 loadTextures 或 initMapLayout 有问题
                // 可以在这里打印一个警告信息方便调试
                // std::cerr << "Warning: Texture not found for tile ID " << tileID << " at (" << x << "," << y << ")" << std::endl;
            }
        }
    }
}

bool Map::isTileWalkable(int tileX, int tileY) const {
    if(tileX < 0 || tileY < 0 || tileX >= m_mapWidth || tileY >= m_mapHeight) {
        return false;
    }
    std::cout << tileX << " " << tileY << std::endl;
    std::cout << "tileID: " << m_layout[tileY][tileX] << std::endl;
    int tileID = m_layout[tileY][tileX];
    return tileID == 0; // 0 表示可通行
}



