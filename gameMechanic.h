//
// Created by netfl on 21.12.2024.
//
#include <iostream>
#include <mutex>
#include <vector>
#include <string>
#include <mutex>
#include <memory>
#include <asio.hpp>
using asio::ip::tcp;

#ifndef GAMEMECHANIC_H

extern int timer;

struct gameData {
    int id;
    std::string name;
    std::string imageLink;
    std::string type;
};

struct player {
    std::string name;
    int score = 0;
    bool answered = false;
    bool connected = false;
    std::chrono::steady_clock::time_point lastActive;
    std::optional<tcp::socket> socket;  // Changed this line
    std::mutex playerMutex;
};

int addPlayer(std::string name);
gameData GameInfo(const std::string& line);
std::string SelectedGame(std::vector<std::string>& vl);
bool answerStatus(std::string answer);
// players table
extern std::vector<std::shared_ptr<player>> players;


#define GAMEMECHANIC_H


#endif //GAMEMECHANIC_H
