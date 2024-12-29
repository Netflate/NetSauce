#include "Game.h"
#include "gameMechanic.h"

#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include <thread>

bool gameStarted = false;

void Game::run() {
    std::cout << "starting the game" << std::endl;
}


void start() {
    // Selecting the question
    std::vector<std::string> vl = getVariantList();
    gameData data = GameInfo(SelectedGame(vl));
    std::cout << "selected game info\n Name: " << data.name << "\n ImageSource: " << data.imageLink << "\n id: " << data.id << "\n type: " << data.type << std::endl;
    // timer
    auto startTime = std::chrono::steady_clock::now();
    auto endTime = startTime + std::chrono::seconds(timer);
    bool timeRunning = true;
    while (timeRunning) {
        std::cout << "testing" << std::endl;
        if (std::chrono::steady_clock::now() > endTime) {
            timeRunning = false;
        }


    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

}



std::vector<std::string> getVariantList() {
    static std::vector<std::string> lines;
    std::ifstream vars("C:/Games/NetSauce/games.txt");
    if(vars){
        //filling vl
        std::string line;
        while (std::getline(vars, line)){
            lines.push_back(line);
        }
    }
    else {
        std::cerr << "Error opening file" << std::endl;
    };
    return lines;
}



void lobby() {
}