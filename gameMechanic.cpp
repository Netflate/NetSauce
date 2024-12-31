#include "gameMechanic.h"
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <iostream>
#include <fstream>
#include <utility>
#include <vector>
#include <sstream>

int timer = 15;
std::vector<std::shared_ptr<player>> players;

gameData GameInfo(const std::string& line){
  gameData data;
  std::string temp;
  std::istringstream stream(line);
  std::getline(stream, temp, ';');
  data.id = std::stoi(temp);
  std::getline(stream, data.imageLink, ';');
  std::getline(stream, data.type, ';');
  std::getline(stream, data.name, ';');
  return data;
}

/*
struct player {
std::string name;
int score = 0;
bool answered = false;
};
*/
int addPlayer(std::string name) {
  auto p = std::make_shared<player>();
  p->name = std::move(name);
  p->connected = true;
  p->lastActive = std::chrono::steady_clock::now();
  p->ping = "69";
  players.push_back(p); // Добавляем shared_ptr в контейнер
  return (players.size() - 1);
}



std::string SelectedGame(std::vector<std::string>& vl){
  int total, sel;
  total = vl.size();
  sel = rand() % total;
  std::string line = vl[sel];
  //removing it from vector to avoid repetitions
  vl[sel] = vl.back();
  vl.pop_back();
  return line;
}

bool answerStatus(std::string answer) {

}
