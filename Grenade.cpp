#include "Grenade.h"
#include "Game.h"       // For Game context
#include "PlayerTank.h" // To identify the player tank
// #include "AITank.h"  // Not strictly needed if only distinguishing player vs non-player
#include <algorithm>    // For std::remove_if
#include <iostream>     // For std::cout, std::cerr

GrenadeTool::GrenadeTool(sf::Vector2f pos, const sf::Texture &texture) : Tools(pos, texture) {
    // std::cout << "GrenadeTool created at (" << pos.x << ", " << pos.y << ")" << std::endl;
}

// tankInteracted is the tank that picked up the tool
void GrenadeTool::applyEffect(Tank &tankInteracted, Game &gameContext) {
    std::cout << "GrenadeTool effect activated by a tank!" << std::endl;

    std::vector<std::unique_ptr<Tank>>& allTanks = gameContext.getAllTanksForModification();
    PlayerTank* playerTankPtrFromContext = gameContext.getPlayerTank(); // Renamed for clarity

    // The tank that picked up the tool might be the player.
    // The current logic spares only the playerTank obtained from gameContext.getPlayerTank().

    if (!playerTankPtrFromContext) {
        std::cerr << "GrenadeTool::applyEffect Error: PlayerTank instance not found in Game context. Cannot apply effect correctly." << std::endl;
        this->setActive(false); // Consume the tool even if effect fails partially
        return;
    }

    allTanks.erase(
            std::remove_if(allTanks.begin(), allTanks.end(),
                           [&](const std::unique_ptr<Tank>& tankToCheck) {
                               // If the tank to check is NOT the player tank (identified by playerTankPtrFromContext), it should be removed.
                               if (tankToCheck.get() != playerTankPtrFromContext) {
                                   if (tankToCheck) { // Ensure the tank unique_ptr is not null
                                       std::cout << "GrenadeTool: Removing tank at (" << tankToCheck->get_position().x
                                                 << ", " << tankToCheck->get_position().y << ")" << std::endl;
                                   }
                                   return true; // Mark for removal
                               }
                               return false; // Keep this tank (it's the player identified via game context)
                           }),
            allTanks.end());

    std::cout << "GrenadeTool effect applied. Non-player tanks (potentially all except one player) removed." << std::endl;
    this->setActive(false); // Mark the tool as used up
}