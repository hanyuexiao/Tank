// Map.cpp
#include "Map.h"
#include "Game.h" // 包含 Game.h 以便使用 Game& game 或 Game* game
#include <iostream>
#include <vector>       // 确保包含
#include <algorithm>    // For std::fill, std::shuffle, std::min, std::max
#include <random>       // For std::mt19937, std::uniform_int_distribution

// =========================================================================
// 构造函数
// =========================================================================
Map::Map() : m_tileWidth(0), m_tileHeight(0),
             m_mapWidth(24), m_mapHeight(15), // 默认地图尺寸 (格子数)
             m_baseHealth(BASE_INITIAL_HEALTH),
             m_isBaseDestroyed(false) {
    // std::cout << "Map constructor called." << std::endl;
}

// =========================================================================
// 初始化与加载方法
// =========================================================================
bool Map::loadDimensionsAndTextures(Game& game) {
    // 从 Game 对象获取图块尺寸信息 (基于一个标准瓦片，如草地)
    const sf::Texture& sampleTexture = game.getTexture("map_grass"); // 假设 "map_grass" 是草地瓦片的键

    if (sampleTexture.getSize().x == 0 || sampleTexture.getSize().y == 0) {
        std::cerr << "Map::loadDimensionsAndTextures() - Error: Could not get valid sample texture ('map_grass') from Game object to determine tile size." << std::endl;
        // 设置一个默认的回退值
        m_tileWidth = 50;
        m_tileHeight = 50;
        std::cerr << "Map::loadDimensionsAndTextures() - Using default tile size: " << m_tileWidth << "x" << m_tileHeight << std::endl;
        // return false; // 如果无法确定图块尺寸则加载失败
    } else {
        m_tileWidth = sampleTexture.getSize().x;
        m_tileHeight = sampleTexture.getSize().y;
    }

    // 实际地图的格子数，可以固定或从配置读取
    // 游戏区域宽度 1200px / 瓦片宽度 (例如 50px) = 24 tiles
    // 窗口高度 750px / 瓦片高度 (例如 50px) = 15 tiles
    if (m_tileWidth > 0) m_mapWidth = 1200 / m_tileWidth; else m_mapWidth = 24;
    if (m_tileHeight > 0) m_mapHeight = 750 / m_tileHeight; else m_mapHeight = 15;


    std::cout << "Map dimensions set: " << m_mapWidth << "x" << m_mapHeight
              << " tiles. Tile size: " << m_tileWidth << "x" << m_tileHeight << std::endl;

    // (可选) 验证所有必需的地图纹理是否已在 Game 中加载
    // Game::getTexture() 在找不到纹理时应该已经有错误处理。
    // 例如:
    if (game.getTexture("map_brick_wall").getSize().x == 0) {
        std::cerr << "Map::loadDimensionsAndTextures() - Warning: Brick wall texture ('map_brick_wall') seems to be missing or invalid." << std::endl;
    }
    if (game.getTexture("map_water").getSize().x == 0) {
        std::cerr << "Map::loadDimensionsAndTextures() - Warning: Water texture ('map_water') seems to be missing or invalid." << std::endl;
    }
    if (game.getTexture("map_forest").getSize().x == 0) {
        std::cerr << "Map::loadDimensionsAndTextures() - Warning: Forest texture ('map_forest') seems to be missing or invalid." << std::endl;
    }


    return true; // 所有加载步骤完成
}

void Map::initializeTileHealth() {
    if (m_layout.empty()) {
        // std::cerr << "Map::initializeTileHealth() - Warning: Layout is empty. Cannot initialize tile health." << std::endl;
        return;
    }
    // 确保 m_tileHealth 与 m_layout 尺寸一致
    m_tileHealth.assign(m_mapHeight, std::vector<int>(m_mapWidth, 0));

    for (int y = 0; y < m_mapHeight; ++y) {
        for (int x = 0; x < m_mapWidth; ++x) {
            if (m_layout[y][x] == 1) { // 砖墙 (ID 1)
                m_tileHealth[y][x] = BRICK_INITIAL_HEALTH;
            }
            // 其他类型的瓦片目前没有独立的生命值系统（除了基地）
        }
    }
}

void Map::generateLayout(int level, std::mt19937& rng, const Game& game) {
    std::cout << "Generating layout for Level " << level << std::endl;

    // 确保地图尺寸已设置
    if (m_mapWidth <= 0 || m_mapHeight <= 0) {
        std::cerr << "Map::generateLayout() Error: Map dimensions are not set or invalid (W:" << m_mapWidth << ", H:" << m_mapHeight << ")." << std::endl;
        // 尝试从game对象获取一次，如果之前失败了
        // const_cast<Game&>(game).getMap().loadDimensionsAndTextures(const_cast<Game&>(game)); // 不太好的做法
        // 更好的做法是在Game::setupLevel之前确保loadDimensionsAndTextures已成功
        if (m_mapWidth <=0 || m_mapHeight <=0) return; // 如果仍然无效，则无法生成
    }


    m_layout.assign(m_mapHeight, std::vector<int>(m_mapWidth, 0)); // 全部置为草地 (ID 0)

    // 1. 绘制边界 (钢墙 ID 2)
    for (int y = 0; y < m_mapHeight; ++y) {
        for (int x = 0; x < m_mapWidth; ++x) {
            if (y == 0 || y == m_mapHeight - 1 || x == 0 || x == m_mapWidth - 1) {
                m_layout[y][x] = 2; // 钢墙
            }
        }
    }

    // 2. 放置基地 (ID 3) 和基础保护
    sf::Vector2i basePos = getBaseTileCoordinate();
    if (basePos.x != -1 && basePos.y != -1 &&
        basePos.x < m_mapWidth && basePos.y < m_mapHeight &&
        basePos.x >= 0 && basePos.y >=0 ) { // 额外边界检查
        m_layout[basePos.y][basePos.x] = 3; // 基地核心

        // 周围一圈砖墙 (ID 1) - 确保不越界
        int bx = basePos.x;
        int by = basePos.y;
        if (by - 1 >= 0) m_layout[by - 1][bx] = 1;
        if (by - 1 >= 0 && bx - 1 >= 0) m_layout[by - 1][bx - 1] = 1;
        if (by - 1 >= 0 && bx + 1 < m_mapWidth) m_layout[by - 1][bx + 1] = 1;
        if (bx - 1 >= 0) m_layout[by][bx - 1] = 1;
        if (bx + 1 < m_mapWidth) m_layout[by][bx + 1] = 1;
    } else {
        std::cerr << "Error: Could not determine valid base position for map generation! BasePos: (" << basePos.x << "," << basePos.y << ")" << std::endl;
        // 放置一个绝对安全的默认位置的基地以防万一
        if (m_mapHeight > 1 && m_mapWidth > 2) {
            m_layout[m_mapHeight-1][m_mapWidth/2] = 3;
        }
    }

    // 3. 根据关卡添加随机元素
    std::uniform_int_distribution<> distribTileType(0, 99); // 0-99 for percentage-like probability

    for (int y = 1; y < m_mapHeight - 1; ++y) { // 不在边界上生成
        for (int x = 1; x < m_mapWidth - 1; ++x) {
            if (m_layout[y][x] != 0) continue; // 如果已被基地或其保护占据，或已是边界，则跳过

            int randomValue = distribTileType(rng);

            if (level == 1) {
                if (randomValue < 25) m_layout[y][x] = 1; // 25% 砖墙
                else if (randomValue < 30) m_layout[y][x] = 2; // 5% 钢墙
            } else if (level == 2) {
                if (randomValue < 20) m_layout[y][x] = 1; // 20% 砖墙
                else if (randomValue < 25) m_layout[y][x] = 2; // 5% 钢墙
                else if (randomValue < 40) m_layout[y][x] = 4; // 15% 水池 (ID 4)
            } else if (level >= 3) {
                if (randomValue < 15) m_layout[y][x] = 1; // 15% 砖墙
                else if (randomValue < 20) m_layout[y][x] = 2; // 5% 钢墙
                else if (randomValue < 35) m_layout[y][x] = 4; // 15% 水池
                else if (randomValue < 50) m_layout[y][x] = 5; // 15% 森林 (ID 5)
            }
        }
    }

    // 4. 确保基地上方有一些空间 (简单清除)
    if (basePos.x != -1 && basePos.y != -1) {
        for(int dy = -3; dy <= -1; ++dy) { // 基地正上方三格
            for(int dx = -2; dx <= 2; ++dx) { // 基地左右两格范围内
                int curX = basePos.x + dx;
                int curY = basePos.y + dy;
                // 确保在内部地图区域
                if(curX > 0 && curX < m_mapWidth -1 && curY > 0 && curY < m_mapHeight -1) {
                    // 不清除紧邻基地的保护墙 (y = basePos.y - 1, x = basePos.x +/- 1 or basePos.x)
                    bool isBaseProtection = (curY == basePos.y - 1 && curX >= basePos.x - 1 && curX <= basePos.x + 1) ||
                                            (curY == basePos.y && (curX == basePos.x -1 || curX == basePos.x+1));
                    if (!isBaseProtection && m_layout[curY][curX] != 3) { // 不是基地本身且不是保护墙
                        m_layout[curY][curX] = 0; // 清空为草地
                    }
                }
            }
        }

    }
    // TODO: 实现更高级的随机地图生成算法以确保可玩性和多样性。
    // 例如:
    // - 使用 Cellular Automata 生成洞穴状结构。
    // - Perlin Noise 生成更自然的区块。
    // - Drunkard's Walk 算法创建路径和开放区域。
    // - 确保基地到AI出生点有通路，玩家出生点到活动区域有通路。
    // - 避免完全封死基地。

    // 5. 确保玩家出生区域的开放性 (例如左下角附近)
    //    这只是一个示例，你可能需要更复杂的逻辑来定义“玩家出生区域”
    int playerSpawnRegionX_start = 1; // 从地图左侧第2列开始
    int playerSpawnRegionY_start = m_mapHeight - 4; // 从地图底部倒数第4行开始 (给基地留空间)
    int playerSpawnRegionWidth = 5;  // 5格宽
    int playerSpawnRegionHeight = 3; // 3格高

    for (int y_offset = 0; y_offset < playerSpawnRegionHeight; ++y_offset) {
        for (int x_offset = 0; x_offset < playerSpawnRegionWidth; ++x_offset) {
            int curX = playerSpawnRegionX_start + x_offset;
            int curY = playerSpawnRegionY_start + y_offset;

            // 确保在地图内部边界内 (不包括最外层墙)
            if (curX > 0 && curX < m_mapWidth - 1 && curY > 0 && curY < m_mapHeight - 1) {
                // 检查这里是否是基地或基地保护的一部分，如果是则不清除
                bool isBaseArea = false;
                if (basePos.x != -1 && basePos.y != -1) {
                    if (std::abs(curX - basePos.x) <= 1 && std::abs(curY - basePos.y) <= 1) {
                        isBaseArea = true; // 基地核心或紧邻的保护
                    }
                }
                if (!isBaseArea && m_layout[curY][curX] != 3) { // 不是基地核心
                    m_layout[curY][curX] = 0; // 设置为草地 (可通行)
                }
            }
        }
    }
    std::cout << "Cleared a potential player spawn region." << std::endl;

    initializeTileHealth(); // 根据新布局设置砖墙血量
    std::cout << "Map layout generated for level " << level << "." << std::endl;
}

void Map::resetForNewLevel() {
    m_baseHealth = BASE_INITIAL_HEALTH;
    m_isBaseDestroyed = false;
    // 布局已由 generateLayout 处理，这里主要是重置状态
    initializeTileHealth(); // 确保砖墙血量基于当前布局被重置
    std::cout << "Map state (base health, tile health) reset for new level." << std::endl;
}

// =========================================================================
// Getter 方法
// =========================================================================
sf::Vector2i Map::getBaseTileCoordinate() const {
    // 基地通常在底部中间 (最底一行的中间)
    if (m_mapWidth > 0 && m_mapHeight > 0) {
        return sf::Vector2i(m_mapWidth / 2, m_mapHeight - 1);
    }
    // std::cerr << "Warning: getBaseTileCoordinate called with uninitialized or invalid map dimensions (W:"<< m_mapWidth << ", H:"<< m_mapHeight <<")." << std::endl;
    return sf::Vector2i(-1, -1); // 表示无效或未找到
}

int Map::getTileType(int tileX, int tileY) const {
    if(tileX < 0 || tileY < 0 || tileX >= m_mapWidth || tileY >= m_mapHeight) {
        // std::cerr << "Map::getTileType Error: Coordinates (" << tileX << "," << tileY << ") are out of bounds." << std::endl;
        return -1; // 超出边界返回无效类型
    }
    return m_layout[tileY][tileX];
}

int Map::getTileHealth(int tileX, int tileY) const {
    if(tileX < 0 || tileY < 0 || tileX >= m_mapWidth || tileY >= m_mapHeight) {
        return -1; // 越界
    }
    if (m_layout[tileY][tileX] == 1) { // 只有砖墙有这个独立的血量记录
        return m_tileHealth[tileY][tileX];
    }
    return 0; // 其他类型的瓦片（如草地、钢墙）可以认为没有这种意义上的“血量”
}

bool Map::isTileWalkable(int tileX, int tileY) const {
    if(tileX < 0 || tileY < 0 || tileX >= m_mapWidth || tileY >= m_mapHeight) {
        return false; // 超出边界
    }
    int tileID = m_layout[tileY][tileX];
    // 瓦片类型ID:
    // 0: Grass (可通行)
    // 1: Brick Wall (不可通行)
    // 2: Steel Wall (不可通行)
    // 3: Base (不可通行)
    // 4: Water (不可通行)
    // 5: Forest (可通行)
    return (tileID == 0 || tileID == 5);
}

int Map::getBaseHealth() const {
    return m_baseHealth;
}

bool Map::isBaseDestroyed() const {
    return m_isBaseDestroyed;
}

// =========================================================================
// 状态修改方法
// =========================================================================
void Map::damageTile(int tileX, int tileY, int damage, Game& game) {
    if (tileX < 0 || tileY < 0 || tileX >= m_mapWidth || tileY >= m_mapHeight) {
        return; // 超出边界
    }

    if (m_layout[tileY][tileX] == 1) { // 如果是砖墙 (ID 1)
        if (m_tileHealth[tileY][tileX] > 0) {
            m_tileHealth[tileY][tileX] -= damage;
            // std::cout << "Brick at (" << tileX << "," << tileY << ") damaged. Health: " << m_tileHealth[y][x] << std::endl;

            if (m_tileHealth[tileY][tileX] <= 0) {
                m_layout[tileY][tileX] = 0; // 变为草地/空格 (ID 0)
                m_tileHealth[tileY][tileX] = 0; // 确保健康值不为负
                std::cout << "Brick at (" << tileX << "," << tileY << ") destroyed." << std::endl;
                // 这里可以通知 Game 更新寻路或其他游戏逻辑，如果需要的话
            }
        }
    }
    // 其他类型的瓦片（如钢墙）目前不受子弹伤害，基地有单独的damageBase方法
}

void Map::damageBase(int damage) {
    if (!m_isBaseDestroyed) {
        m_baseHealth -= damage;
        std::cout << "Base damaged by " << damage << ". Current Base Health: " << m_baseHealth << std::endl;
        if (m_baseHealth <= 0) {
            m_baseHealth = 0;
            m_isBaseDestroyed = true;
            std::cout << "Base DESTROYED!" << std::endl;
            // Game Over 逻辑将在 Game 类中处理 (通过检查 isBaseDestroyed())
        }
    }
}

// =========================================================================
// 绘制方法
// =========================================================================
void Map::draw(sf::RenderWindow &window, Game& game) {
    if (m_layout.empty() || m_tileWidth == 0 || m_tileHeight == 0) {
        // 如果地图未初始化或图块尺寸未知，则不绘制
        // std::cerr << "Map::draw() - Warning: Layout empty or tile dimensions zero. Skipping draw." << std::endl;
        return;
    }

    // 遍历地图布局
    for (int y = 0; y < m_mapHeight; ++y) {
        for (int x = 0; x < m_mapWidth; ++x) {
            int tileID = m_layout[y][x];
            std::string textureKey;

            if (tileID == 1) { // 如果是砖墙 (ID 1)，根据血量选择不同纹理
                int health = m_tileHealth[y][x]; // 注意：这里之前有个笔误，应该是 m_tileHealth[y][x]
                if (health >= BRICK_INITIAL_HEALTH) { // 满血或更高 (以防万一)
                    textureKey = "map_brick_wall";
                } else if (health == 2) {
                    textureKey = "map_brick_wall_damaged1"; // 假设你已将其添加到 config.json
                } else if (health == 1) {
                    textureKey = "map_brick_wall_damaged2"; // 假设你已将其添加到 config.json
                } else { // health <= 0, 砖墙已被摧毁，逻辑上tileID应该已经是0了，这里为了安全显示为草地
                    textureKey = "map_grass";
                    if (m_layout[y][x] != 0) { // 如果布局ID还未更新为0，是个逻辑问题
                        // std::cout << "Warning: Brick at (" << x << "," << y << ") has health " << health << " but layout ID is still 1." << std::endl;
                    }
                }
            } else {
                // 将 tileID 映射到 Game 缓存中纹理的键名
                switch (tileID) {
                    case 0: textureKey = "map_grass"; break;
                    case 2: textureKey = "map_steel_wall"; break;
                    case 3: textureKey = "map_base"; break;
                    case 4: textureKey = "map_water"; break;   // 新增水
                    case 5: textureKey = "map_forest"; break;  // 新增森林
                    default:
                        // std::cerr << "Map::draw() Warning: Unknown tile ID " << tileID << " at (" << x << "," << y << ")" << std::endl;
                        continue; // 跳过未知ID 或绘制一个默认的错误图块
                }
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
