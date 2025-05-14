// main.cpp
#include "Game.h" // 假设你的Game类定义在Game.h中
#include <iostream>

int main() {
    Game game; // 创建 Game 对象，构造函数会被调用
    game.init(); // Game的构造函数现在调用init，所以这里不需要显式调用（除非你改了逻辑）

    if (game.isWindowOpen()) { // 假设你给Game类加一个isWindowOpen()方法，或者直接在run里判断
        game.run(); // 启动游戏主循环
    } else {
        std::cerr << "Failed to initialize the game or window could not be opened." << std::endl;
        return -1;
    }
    return 0;
}