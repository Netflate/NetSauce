//
// Created by netfl on 21.12.2024.
//
#include <vector>
#include <string>

#ifndef GAME_H
#define GAME_H

extern bool gameStarted;


class Game {
    public:
    void run();
};
void lobby();
void start();
std::vector<std::string> getVariantList();


#endif //GAME_H
