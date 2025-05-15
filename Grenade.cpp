//
// Created by admin on 2025/5/14.
//

#include "Grenade.h"
#include "Game.h"
#include "PlayerTank.h"
#include <algorithm>

GrenadeTool::GrenadeTool(sf::Vector2f pos, const sf::Texture &texture) : Tools(pos, texture) {
    std::cout << "GrenadeTool created" << std::endl;
}

void GrenadeTool::applyEffect(Tank &tank, Game &gameContext) {
    std::cout << "Applying GrenadeTool effect" << std::endl;
    std::vector<std::unique_ptr<Tank>>& allTanks = gameContext.getAllTanksForModification();
    PlayerTank* playerTank = gameContext.getPlayerTank();

    if(!playerTank){
        std::cerr << "GrenadeTool::applyEffect: playerTank is nullptr" << std::endl;
    }

    allTanks.erase(
            std::remove_if(allTanks.begin(), allTanks.end(),
                           [&](const std::unique_ptr<Tank>& tankToCheck) {
                if(tankToCheck.get() != playerTank){
                    std::cout << "GrenadeTool::applyEffect: removing tank" << std::endl;

                    return true;
                }
                return false;
            }),
            allTanks.end());

    std::cout << "GrenadeTool effect applied" << std::endl;
    this ->setActive(false);
}
