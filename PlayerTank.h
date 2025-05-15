//
// Created by admin on 2025/5/12.
//
#include "tank.h"

#ifndef TANKS_PLAYERTANK_H
#define TANKS_PLAYERTANK_H

class PlayerTank : public Tank {
public:
    PlayerTank(sf::Vector2f startPosition, Direction startDirection, int frameWidth = 50, int frameHeight = 50,int inihealth=100);
    // PlayerTank 可能会有自己特定的纹理加载逻辑或构造参数，如果它的外观/尺寸与默认 Tank 不同
    // 例如：PlayerTank(sf::Vector2f startPosition, Direction startDirection, const std::string& playerTexturePrefix);

    // PlayerTank 特有的方法可以放在这里，例如特殊技能
    // void activateShield();
};

#endif //TANKS_PLAYERTANK_H
