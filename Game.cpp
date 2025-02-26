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

//==================================
// COLOR DEFINITIONS
//==================================
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


//==================================
// Getters and Setters
//==================================


std::string game::gameCore::getTrueAnswer() const {
    return trueAnswer;
}

bool game::gameCore::getGameEnd() const {
    return end;
}


float game::gameCore::getHowMuchTimeLeft() const {
    return timeLeft;
}

void game::gameCore::setWinningScore(const int score) {
    winningScore = score;
}

int game::gameCore::getWinningScore() const {
    return winningScore;
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


void game::gameCore::setTimer(int t) {
    timer = t;
}


void game::gameCore::setCat(const std::unordered_set<std::string>& cats) {
    categories = cats;
}

void game::gameCore::setType(const std::unordered_set<std::string>& types) {
    questions_type = types;
}

//=================================================
// MAIN FUNCTION OF THE FILE, RUNNING GAME FUNCTION
//===================================================

void game::gameCore::launchRound() {
    end = false;                                                            // END OF THE WHOLE GAME, NOT CURRENT ROUND

    setType({"image", "gif", "video"});                    // I'll leave it hard programmed ig, not quite interested in changing it, as there isn't many questions anyway
    setTimer(15);
    //std::cout
    //        << CYAN
    //        << "ROUND STARTED \n"
    //        << RESET ;

    std::unordered_map<int, std::string> variantList = getVariantList();   // getting the list of all

    while (!end) { // here the game begins
        if (parent->connectedPlayers == 0) {end = true;}                  // stopping the game, if there isnt any connected players
        answering = parent->connectedPlayers;                             // answering takes the number of connected players
        parent->skippingPlayers = 0 ;                                     // players voting to skip the question, the question get skipped after all remaining answering players vote to skip

        if (variantList.empty()) {                                         // if there is no questions, the game simply ends
            //std::cout
            //    << RED
            //    << "THERE ISN'T ANY QUESTIONS LEFT, SORRY\n"
            //    << RESET ;
            end = true;
            break;
        }

        bool roundEnd = true;                                              // bool that represents if the round has to end
        gameData currentQuestion = GameInfo(SelectedGame(variantList)); // current Question

        trueAnswer = currentQuestion.name;                                 // true answer takes the game name for checking if player's answers are right
        trueAnswerAbbreviation = currentQuestion.abbreviation;             // same with her, but with game's name abbreviation

            auto start = std::chrono::steady_clock::now(); // starting round time
            auto update = start;                           // another time saver to send to client how much time left
            timeLeft = static_cast<float>(timer);                              // copy of timer, but now in float



            /*********      SENDING THE CLIENT LINK TO THE (IMAGE/QUESTION) link, or question itself, if its text type
             *                               Along with question hint and question type itself                         **********/


            if (currentQuestion.question_type == "video") {                    // if it's a video question, it sends to the timing, when the video should stop
                paramSignal("questionInfo:" + currentQuestion.question_type +"li%^nk:" +currentQuestion.link + "hi%^nt:" + currentQuestion.description + "se%^ds:" + currentQuestion.pause);
                questionInfo = "questionInfo:" + currentQuestion.question_type +"li%^nk:" +currentQuestion.link + "hi%^nt:" + currentQuestion.description + "se%^ds:" + currentQuestion.pause;
            }
            else {
                paramSignal("questionInfo:" + currentQuestion.question_type +"li%^nk:" +currentQuestion.link + "hi%^nt:" + currentQuestion.description );
                questionInfo = "questionInfo:" + currentQuestion.question_type +"li%^nk:" +currentQuestion.link + "hi%^nt:" + currentQuestion.description ;
            }
            // saving question info outside of this function, for those players, who connect mid-game

            while (roundEnd) {                                                 // round started
                const auto elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count(); //checking how much time left


                if (( elapsed >= timer ) || answering == 0 || answering == parent->skippingPlayers) {                 // if the timer ends, or all players answer the round ends
                    roundEnd = false;
                }
                if (std::chrono::duration_cast<std::chrono::seconds>( std::chrono::steady_clock::now() - update).count() >= 0.5 ) { // just updating another timer, for the client, every second
                    update = std::chrono::steady_clock::now();
                    timeLeft = std::max(0.0f, static_cast<float>(timer) - static_cast<float>(elapsed));    //this timer represents how much time left, and not how much time passed
                    // just sending to clients how much time left
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(0) << timeLeft;    // converting timeLeft to string
                    std::string str = oss.str();
                    paramSignal("roundTimer:" + str);                                           // sending to client
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        triggerSignal();                                                    // triggering signal that changes answer status of all connected players to false, and reveals all player's answer, as well as shows the real answer
        if (currentQuestion.question_type != "video") {
            paramSignal("endOfTheQuestion:" + trueAnswer);
            std::this_thread::sleep_for(std::chrono::seconds(5));
            paramSignal("resetThQuestion");
        } else {
            paramSignal("endOfTheQuestion:" + trueAnswer);
            std::this_thread::sleep_for(std::chrono::seconds(std::stoi(currentQuestion.show)));
            paramSignal("resetThQuestion");
       }

    triggerSignal();                                                        // If end=true, it instead sends to all clients, that the game has ended

    }
    triggerSignal();                                                        // But now, it only sends to clients the fact, that the game has ended, and the player with the most score
}

//==================================
// UTILITIES
//==================================

// Checking if the answer is right

bool game::gameCore::answer(std::string answer) {

    if (answer == trueAnswer || answer == trueAnswerAbbreviation ||  answer.find(trueAnswer) != std::string::npos) {
        answering -= 1;
        return true;
    }
    else if (normalised(answer) == normalised(trueAnswer) || (normalised(answer)) == normalised(trueAnswer.substr(0, trueAnswer.find(":"))) ) {
        answering -= 1;
        return true;
    }
    return false;
}
// Simplifying player answers to be less strict about exact matches.
std::string game::gameCore::normalised(const std::string& str) {
    std::string normalized;
    std::istringstream iss(str);
    std::string word;

    while (iss >> word) {
        std::string lowerWord;
        for (char c : word) {
            lowerWord.push_back(std::tolower(c));
        }

        // Пропускаем слова "the" и "i"
        if (lowerWord == "the") {
            continue;
        }

        // Обрабатываем символы слова
        for (char c : word) {
            if (c != '\'' && c != ':' && c != ';' && c != '!' && c != '?' && (c > '9' || c < '0')) {
                normalized.push_back(std::tolower(c));
            }
        }
    }

    return normalized;
}

// Parsing game info from string

game::gameCore::gameData game::gameCore::GameInfo(std::string line){
    gameData data;
    std::string temp;
    std::istringstream stream(line);
    std::getline(stream, temp, ';');
    //id
    std::getline(stream, data.question_type, ';'); // type of question (image, video, text and etc )

    std::getline(stream, data.link, ';');    // link to the media corresponding to question
    std::getline(stream, data.question_cat, ';');    // question categorie, either game, flags, movie, or whatever
    std::getline(stream, data.name, ';');    // answer or simply the name
    std::getline(stream, data.description, ';');    // game undirect description, you can call that a hint
    if (data.question_type == "video") {
        std::getline(stream, data.pause, ';');    // Time at which video should be paused, waiting for player's answer
        std::getline(stream, data.show, ';');    // Seconds to show player after the time end, in other words to show him what he had to say
    }
    std::getline(stream, data.abbreviation, ';');    // game name abbreviation, simple example: Counter strike global offensive -> csgo


    return data;
}

// Randomly selecting question

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

    return line;
}

// well, splitting string

std::vector<std::string> game::gameCore::splitString(const std::string& s, char delim) {
    std::vector<std::string> res;
    std::stringstream ss(s);
    std::string item ;
    while (std::getline(ss, item, delim)) {
        res.push_back(item);
    }
    return res;
}


// saving all possible questions in one hash table

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


