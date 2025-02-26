#include "server.h"
#include "Game.h"
#include "gameMechanic.h"

#include <iostream>
#include <thread>
#include <cmath>
#include <asio.hpp>
#include <mutex>
#include <windows.h>
#include <fstream>

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
// GAME MECHANICS AND SIGNAL HANDLING
//==================================
void game::SessionServer::paramSignalHandler(const std::string &stringToSend) {
    broadcasting(stringToSend);
}

void game::SessionServer::signalHandler() {
    if (!gameplay.getGameEnd()) {
        // if the game hasn't ended, as a new question will appear soon
        size_t const n = players.size(); // it sends every player's answer status to false
        for (size_t i = 0; i < n; i++) {
            if (players[i]->getAnswered() == false) {
                broadcasting("ParsePlayerData:" + players[i]->name + "#" + players[i]->getAnswer() + "#" + std::to_string(players[i]->returnScore()));
            }
            else {
                players[i]->setAnswered(false);
                broadcasting("ParsePlayerData:" + players[i]->name + "#!@@true!$!" + players[i]->getAnswer() + "#" + std::to_string(players[i]->returnScore()));

            }
        }
    } else {
        // on the other hand, if the game already ended, we send to everyone that fact, and the winner, which is the one, who has the highest score
        broadcasting("the game has ended:" + winningPlayer());
        // resending player's list, for the lobby menu
        std::this_thread::sleep_for(std::chrono::seconds(11));
        playersBroadcasting();
        players_game_information_reset();
        messaging("Leader:" + questionCatsList(), leader_player_id); // resending the first player info that he is the leader, and can start the game or change its settings
    }
}

void game::SessionServer::players_game_information_reset() {
    for (auto const player : players) {
        player->setScore(0);
    }
}

std::unordered_set<std::string> game::SessionServer::parseCategories(const std::string& categoriesStr, char delimiter) {
    std::unordered_set<std::string> categoriesSet;
    std::stringstream ss(categoriesStr);
    std::string category;

    // Разбиваем строку по разделителю и добавляем каждую категорию в set
    while (std::getline(ss, category, delimiter)) {
        if (!category.empty()) {
            categoriesSet.insert(category);
        }
    }

    return categoriesSet;
}

std::string game::SessionServer::winningPlayer() const {
    int maxScore = 0;
    int id = 0;
    for (size_t i = 0; i < players.size(); i++) {
        if (players[i]->connected) {
            if (players[i]->returnScore() > maxScore) {
                maxScore = players[i]->returnScore();
                id = i;
            }
        }
    }
    return (players[id]->name);
}

std::string game::SessionServer::questionCatsList() {
    std::string lines = "";
    std::ifstream vars("../questions/questions_data.txt");

    if(vars){
        std::string line;
        while (std::getline(vars, line)) {
            auto parts = gameplay.splitString(line, ';');
            if (lines.find(parts[3]) == std::string::npos) {
                lines += parts[3] + '-';
            }
        }
    }
    else {
        std::cerr << "Error opening file" << std::endl;
    }
    return lines;
}

//==================================
// PLAYER ANSWER HANDLING
//==================================
void game::SessionServer::handlingPlayerAnswer(std::string &answer, const int &clientId) {
    if (!players[clientId]->getAnswered()) {
        if (answer == "/skip") {
            parent->skippingPlayers++;
            players[clientId]->setAnswered(true);           // to not allow the player from further answers, he made his call
            broadcasting("ParsePlayerData:" + players[clientId]->name + "#!@@skip!$%" + "#" + std::to_string(players[clientId]->returnScore()));
            // exp "Netflate#Legend Of Zelda: Tears of the kingdom#90#"
            messaging("block", clientId);          // no really necessary; blocking the player from answer but on the client side now
        }
        else {
            players[clientId]->setAnswer(answer);
            if (gameplay.answer(answer)) {
                const float responseTime = static_cast<float>(gameplay.getTimer()) - gameplay.getHowMuchTimeLeft();
                players[clientId]->responseTime = responseTime;
                int toAdd;
                players[clientId]->setAnswered(true);
                if (parent->connectedPlayers - 1 == gameplay.getAnswering()) {
                    toAdd = 10;
                    firstPlayerResponseTime = responseTime;
                } else {
                    float timeInterval = static_cast<float>(gameplay.getTimer()) / 8;
                    toAdd = static_cast<int>(9 - ((responseTime - firstPlayerResponseTime) / timeInterval));
                }
                if (players[clientId]->addnReturnScore(toAdd) >= gameplay.getWinningScore()) { gameplay.setGameEnd(true); }
                broadcasting("ParsePlayerData:" + players[clientId]->name + "#!@@true!$%"  + std::to_string(responseTime) + "#" + std::to_string(players[clientId]->returnScore())); // exp "Netflate#Legend Of Zelda: Tears of the kingdom#90#"
                messaging("block", clientId);          // no really necessary; blocking the player from answer but on the client side now

            }
            else {
                // we just send to all the clients this player answer, as it is false, so its cool
                broadcasting("ParsePlayerData:" + players[clientId]->name + "#" + answer + "#" + std::to_string(players[clientId]->returnScore())); // exp "Netflate#Legend Of Zelda: Tears of the kingdom#90#"
            }
        }
    }
}

//==================================
// PLAYER CLASS METHODS
//==================================
bool game::player::getAnswered() const {
    return answered;
}

void game::player::setAnswered(bool newStatus) {
    answered = newStatus;
}

int game::player::returnScore() const {
    return score;
}

void game::player::setScore(const int sc) {
    score = sc;
}

int &game::player::addnReturnScore(const int &toAdd) {
    score += toAdd;
    return score;
}

std::string game::player::getAnswer() {
    return lastAnswer;
}

void game::player::setAnswer(std::string &answer) {
    lastAnswer = answer;
}

//==================================
// MESSAGING AND BROADCASTING
//==================================
void game::SessionServer::messaging(std::string messageForClient, int i) {
    std::cout << messageForClient << std::endl;
    std::lock_guard<std::mutex> lock(players[i]->playerMutex);
    if (players[i]->connected) {
        try {
            std::cout << messageForClient << std::endl;

            size_t bytes_written = asio::write(*(players[i]->socket), asio::buffer(messageForClient + "~"));
            if (bytes_written > 0) {
                players[i]->lastActive = std::chrono::steady_clock::now();
            }
        } catch (std::system_error &e) {
            std::cerr << "Error writing to socket: " << e.what() << std::endl;
        }
    }
}

void game::SessionServer::broadcasting(std::string messageForClients) {
    int n = players.size();
    std::cout
            << YELLOW
            << messageForClients
            << RESET << "\n";
    for (int i = 0; i < n; i++) {
        messaging(messageForClients, i);
    }
}

void game::SessionServer::ping_broadcasting() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(4));
        int n = players.size();
        if (n > 0) {
            std::string pingList = "playersPing:";
            for (int i = 0; i < n; i++) {
                std::lock_guard<std::mutex> lock(players[i]->playerMutex);
                if (players[i]->connected) {
                    pingList += players[i]->ping + "#";
                }
            }
            broadcasting(pingList);

            if (parent->connectedPlayers <= 0) {
                std::cout << "No players connected. Resetting server..." << std::endl;
                parent->reset();

                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        }
    }
}

void game::SessionServer::playersBroadcasting() {
    std::lock_guard<std::mutex> lock(well_well_well);
    int n = players.size();
    std::string playersListNew = "playersList:";
    int j = 0;
    for (int i = 0; i < n; i++) {
        std::lock_guard<std::mutex> lock(players[i]->playerMutex);
        if (players[i]->connected) {
            playersListNew += players[i]->name + "#";
            if (j == 0) j = i;
        }
    }
    if (playersListNew.substr(0, playersListNew.find("#")) != playersList.substr(0, playersList.find("#"))) {
        messaging("Leader:" + questionCatsList(), j);
        leader_player_id = j;
    }
    playersList = std::move(playersListNew);
    broadcasting(playersList);
}

//==================================
// CLIENT MANAGEMENT
//==================================
void game::SessionServer::monitor_clients(int clientid) {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        if ((std::chrono::steady_clock::now() - players[clientid]->lastActive) > std::chrono::seconds(5)) {
            std::unique_lock<std::mutex> lock(players[clientid]->playerMutex);
            //std::cout << "\n" << RED << players[clientid]->name << " disconnected \n" << RESET;
            players[clientid]->connected = false;
            lock.unlock();
            playersBroadcasting();
            used_nicknames.erase(players[clientid]->name);
            broadcasting("ChatTechnical:" + players[clientid]->name + " left");\
            parent->connectedPlayers--;

            break;
        }
        for (int i = 0; i < players.size(); i++) {
            //std::cout
            //        << PINK
            //        << "\nplayer" << i << ": " << players[i]->name << " |connection status: " << players[i]->connected
            //        << RESET << std::endl;
        }
    }
}

int game::SessionServer::addPlayer(std::string name) {
    auto p = std::make_shared<player>();
    used_nicknames.insert(name);

    p->name = std::move(name);
    p->connected = true;
    p->lastActive = std::chrono::steady_clock::now();
    p->ping = "40";
    players.push_back(p);
    return (players.size() - 1);
}

void game::SessionServer::handle_client(tcp::socket socket) {
    int clientId = -1;
    try {
        while (true) {
            char buffer[1024];
            asio::error_code error;
            tcp::socket &working_socket = (clientId == -1) ? socket : *(players[clientId]->socket);

            size_t length = working_socket.read_some(asio::buffer(buffer), error);
            if (!error) {
                std::string clientMessage(buffer, length);

                // Use a hash map to dispatch messages instead of linear if-else chain
                // First, extract command prefix from the message
                std::string command;
                std::string content;

                if (clientMessage.find("ping") != std::string::npos) {
                    command = "ping";
                } else if (clientMessage.find("Answer:") != std::string::npos) {
                    command = "Answer";
                    content = clientMessage.substr(7); // "Answer:" length
                } else if (clientMessage.find("nicknameCheck:") != std::string::npos) {
                    command = "nicknameCheck";
                    content = clientMessage.substr(14);
                } else if (clientMessage.find("nicknameAdd:") != std::string::npos) {
                    command = "nicknameAdd";
                    content = clientMessage.substr(12);
                } else if (clientMessage.find("startingGame:") != std::string::npos) {
                    command = "startingGame";
                    content = clientMessage.substr(13);
                } else if (clientMessage.find("clientMessage:") != std::string::npos) {
                    command = "clientMessage";
                }

                static const std::unordered_map<std::string, int> commandMap = {
                    {"ping", 1},
                    {"nicknameCheck", 2},
                    {"nicknameAdd", 3},
                    {"startingGame", 4},
                    {"clientMessage", 5},
                    {"Answer", 6}
                };

                auto it = commandMap.find(command);
                int commandCode = (it != commandMap.end()) ? it->second : 0;

                switch (commandCode) {
                    case 1: { // ping
                        std::lock_guard<std::mutex> lock(players[clientId]->playerMutex);
                        asio::write(working_socket, asio::buffer("pong~"));
                        length = working_socket.read_some(asio::buffer(buffer), error);
                        std::string ping(buffer, length);

                        // Add proper error handling for stof
                        try {
                            float pingValue = std::stof(ping);
                            std::string pingstring = std::to_string(std::round(pingValue));
                            players[clientId]->ping = pingstring.substr(0, pingstring.find('.'));
                        } catch (const std::invalid_argument& e) {
                            // Handle invalid ping format
                            players[clientId]->ping = "0";
                        } catch (const std::out_of_range& e) {
                            // Handle out of range ping value
                            players[clientId]->ping = "999";
                        }

                        players[clientId]->lastActive = std::chrono::steady_clock::now();
                        break;
                    }
                    case 2: { // nicknameCheck
                        std::cout << used_nicknames.contains(content) << std::endl;
                        if (!used_nicknames.contains(content)) {
                            asio::write(socket, asio::buffer("OK"));
                        }
                        else {
                            asio::write(socket, asio::buffer("notOK"));
                        }
                        break;
                    }
                    case 3: { // nicknameAdd
                        clientId = addPlayer(content);
                        players[clientId]->socket.emplace(std::move(socket));

                        if (gameStarted == true) {
                            messaging("Started", clientId);
                            messaging(gameplay.questionInfo, clientId);
                        }  // telling the client if the game already started, so it doesn't load pre-game lobby

                        playersBroadcasting();
                        broadcasting("ChatTechnical:" + content + " joined");
                        parent->connectedPlayers++;

                        std::thread monitor_thread(&game::SessionServer::monitor_clients, this, clientId);
                        monitor_thread.detach();
                        break;
                    }
                    case 4: { // startingGame
                        gameplay.setCat(parseCategories(content, '-'));
                        std::thread gameStarting_thread(&game::SessionServer::gameStarting, this);
                        gameStarting_thread.detach();
                        break;
                    }
                    case 5: { // clientMessage
                        broadcasting(clientMessage + "z!z" + players[clientId]->name);
                        break;
                    }
                    case 6: { // Answer
                        handlingPlayerAnswer(content, clientId);
                        break;
                    }
                    default:
                        // No recognized command
                        break;
                }
            } else {
                std::cerr << "Error reading message: " << error.message() << " (Connection failed)" << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(11));
                break;
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "Exception caught: " << e.what() << "\n";
    }
}

//==================================
// GAME SESSION MANAGEMENT
//==================================
void game::SessionServer::gameStarting() {
    int duration = 5;
    for (int i = duration; i > 0; --i) {
        broadcasting("timer:" + std::to_string(i));
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    gameStarted = true;
    broadcasting("Started");
    playersBroadcasting();
    parent->gameCore_instance.launchRound();
}

//==================================
// SERVER INITIALIZATION AND CONNECTION
//==================================
void game::SessionServer::connecting(tcp::acceptor &acceptor, asio::io_context &io_context) {
    try {
        std::cout
               << GRAY
                << "Waiting for client connection..."
                << RESET << "\n";
        acceptor.async_accept([this, &io_context, &acceptor](const asio::error_code &error, tcp::socket socket) {
            if (!error) {
                std::cout
                        << MAGENTA
                        << "Client connected: " << socket.remote_endpoint()
                        << RESET << "\n";
                std::thread(&game::SessionServer::handle_client, this, std::move(socket)).detach();
                connecting(acceptor, io_context);
            } else {
                std::cout << "Error accepting connection: " << error.message() << "\n";
            }
        });

        io_context.run();
    } catch (std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

void game::SessionServer::server_starting() {
    system(("chcp " + std::to_string(CP_UTF8)).c_str());
    try {
        asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(asio::ip::address_v4::any(), 12345));


        std::cout << "Server is running with port: 12345\n";

        std::thread ping_thread(&game::SessionServer::ping_broadcasting, this);
        ping_thread.detach();
        connecting(acceptor, io_context);

        io_context.run();
    } catch (std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}