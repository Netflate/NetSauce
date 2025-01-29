#include "Game.h"
#include "gameMechanic.h"
#include "server.h"
#include <cmath>  // For exp(), log(), sqrt()
#include <random>

#include <iostream>
#include <utility>
#include <vector>
#include <fstream>
#include <chrono>
#include <thread>
#include <filesystem>
#include <bits/random.h>

bool gameStarted = false;
//colors
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"
#define PINK "\033[38;5;213m"
#define GRAY "\033[38;5;250m"




// Методы для работы с категориями, типами вопросов, и ссылкой на актуальный вопрос, чтобы можно было к нему обращаться из функции класса serverSession
float game::gameCore::getHowMuchTimeLeft() const {
    return timeLeft;
}

void game::gameCore::setWinningScore(int score) {
    winningScore = score;
}

int game::gameCore::getAnswering() const {
    return answering;
}


void game::gameCore::setGameEnd(bool) {
    end = true;
}


int game::gameCore::getTimer() const {
    return timer;
}

std::unordered_set<std::string> game::gameCore::getCat() const {
    return categories;
}

std::unordered_set<std::string> game::gameCore::getType() const {
    return questions_type;
}

// Методы для установки значений категорий и типов
void game::gameCore::setTimer(int t) {
    timer = t;
}


void game::gameCore::setCat(const std::unordered_set<std::string>& cats) {
    categories = cats;
}

void game::gameCore::setType(const std::unordered_set<std::string>& types) {
    questions_type = types;
}


void game::gameCore::launchRound() {
    end = false; // END OF THE WHOLE GAME, NOT CURRENT ROUND

    setCat({"anime", "movie", "game"}); // testing, just for now
    setType({"image", "gif", "video"}); //
    setTimer(15);
    std::cout
            << CYAN
            << "ROUND STARTED \n"
            << RESET ;

    std::unordered_map<int, std::string> variantList = getVariantList(); // getting the list of all

    while (!end) {
        answering = parent->connectedPlayers;
        if (variantList.size()==0) {
            // тут наверное надо добавить функцию, которая скажет клиенту, что список закончился;
            std::cout
                << RED
                << "THERE ISN'T ANY QUESTIONS LEFT, SORRY\n"
                << RESET ;
            end = true;
            break;
        }
        std::string answer; // в будущем клиент будет посылать на сервак answer:example, и вот как раз отсюда и появится переменная answer
        bool roundEnd = true;
        gameData currentQuestion = GameInfo(SelectedGame(variantList));
        trueAnswer = currentQuestion.name;
        
        auto start = std::chrono::steady_clock::now();
        auto update = start;
        timeLeft = static_cast<float>(timer);
        while (roundEnd) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count();

            if (( elapsed >= timer ) || answering == 0 ) {
                roundEnd = false;
            }
            if (std::chrono::duration_cast<std::chrono::seconds>( std::chrono::steady_clock::now() - update).count() > 1 ) {
                update = std::chrono::steady_clock::now();
                timeLeft = std::max(0.0f, static_cast<float>(timer) - static_cast<float>(elapsed));

            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        triggerSignal();  // triggering signal that changes answer status of all connected players to false
        // nickname:testing
        // startingGame
        // Answer:
    }
}


bool game::gameCore::answer(std::string answer) {

    if (answer == trueAnswer || answer.find(trueAnswer) != std::string::npos) {
        std::cout << "\n right answer; \n" ;
        answering -= 1;
        return true;
    }
    else if (normalised(answer) == normalised(trueAnswer) || (normalised(answer)) == normalised(trueAnswer.substr(0, trueAnswer.find(":"))) ) {
        std::cout << "\n right answer; \n" ;
        answering -= 1;
        return true;
    }
    std::cout << "\n wrong answer; \n" ;
    return false;
}

std::string game::gameCore::normalised(const std::string& str) {
    std::string normalized ;
    for (const auto& c : str) {
        if (c != '\'' && c != ' ' && c != ':' && c != ';' && (c > '9' || c < '0') ) {
            normalized.push_back(tolower(c));
        }
    }
    return normalized;
}

game::gameCore::gameData game::gameCore::GameInfo(const std::string line){
    game::gameCore::gameData data;
    std::string temp;
    std::istringstream stream(line);
    std::getline(stream, temp, ';');
    //id
    // type of question (image, video, text and etc )
    std::getline(stream, data.question_type, ';');
    // link to the media corresponding to question
    std::getline(stream, data.link, ';');
    // question categorie, either game, flags, movie, or whatever
    std::getline(stream, data.question_cat, ';');
    // answer or simply the name
    std::getline(stream, data.name, ';');

    return data;
}

std::string game::gameCore::SelectedGame(std::unordered_map<int, std::string>& variantList){
    std::random_device rd;     // True random seed source
    std::mt19937 gen(rd());    // Mersenne Twister generator
    std::uniform_int_distribution<> distrib(0, variantList.size() - 1);  // Distribution range

    // Random number generation steps:
    int randomIndex = distrib(gen);

    auto it = variantList.begin();
    std::advance(it, randomIndex);

    std::string line = it->second;
    variantList.erase(it); // removing used question, to avoid repetitions
    std:: cout << "line: " << line << std::endl;
    return line;
}


std::vector<std::string> game::gameCore::splitString(const std::string& s, char delim) {
    std::vector<std::string> res;
    std::stringstream ss(s);
    std::string item ;
    while (std::getline(ss, item, delim)) {
        res.push_back(item);
    }
    return res;
}



std::unordered_map<int, std::string> game::gameCore::getVariantList() {
    std::unordered_map<int, std::string> lines;
    std::ifstream vars("../questions/questions_data.txt");

    if(vars){
        std::string line;
        int count = 0 ;
        while (std::getline(vars, line)) {
            auto parts = splitString(line, ';');
            if (getCat().find(parts[3]) != getCat().end() && getType().find(parts[1]) != getType().end()) {
                lines.emplace(count, line);
                count++;
            }
        }
    }
    else {
        std::cerr << "Error opening file" << std::endl;
    };
    return lines;
}


