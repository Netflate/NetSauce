#include "server.h"
#include "Game.h"
#include "gameMechanic.h"

#include <iostream>
#include <thread>
#include <cmath>
#include <asio.hpp>
#include <mutex>
#include <windows.h>

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


using asio::ip::tcp;


void game::SessionServer::signalHandler() {
    if (!gameplay.getGameEnd()) {               // if the game hasn't ended, as a new question will appear soon
        size_t const n = players.size();        // it sends every player's answer status to false
        for (size_t i = 0; i < n; i++) {        // and sends them true answer to the last question
            players[i]->setAnswered(false);  // answerStatus
        }
        broadcasting(gameplay.getTrueAnswer()); // trueAnswer
    }
    else {                                      // on the other hand, if the game already ended, we send to everyone that fact, and the winner, which is the one, who has the highest score
        broadcasting("the game has ended");
        broadcasting(winningPlayer());
    }
}

std::string game::SessionServer::winningPlayer() const {
    int maxScore = 0;
    int id = 0 ;
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




void game::SessionServer::handlingPlayerAnswer( std::string& answer, const int& clientId) {
    if (!players[clientId]->getAnswered()) {
        players[clientId]->setAnswer(answer);
        if (gameplay.answer(answer)) {
            const float responseTime = static_cast<float>(gameplay.getTimer()) - gameplay.getHowMuchTimeLeft();
            players[clientId]->responseTime = responseTime;
            int toAdd ;
            players[clientId]->setAnswered(true);
            if (parent->connectedPlayers - 1 == gameplay.getAnswering()) {
                toAdd = 10;
                firstPlayerResponseTime = responseTime ;
            }
            else {
                float timeInterval = static_cast<float>(gameplay.getTimer()) / 8;
                toAdd = static_cast<int>(9 - ((responseTime - firstPlayerResponseTime) / timeInterval));
            }
            if (players[clientId]->addnReturnScore(toAdd) >= gameplay.getWinningScore()) { gameplay.setGameEnd(true); }
        }
    }
}

// functions related to class player variables, which are player's score, answer, and answer status

bool game::player::getAnswered() const {
    return answered;
}

void game::player::setAnswered(bool newStatus) {
    answered = newStatus;
}


int game::player::returnScore() const {
    return score;
}

int& game::player::addnReturnScore(const int& toAdd) {
    score+= toAdd;
    return score;
}

std::string game::player::getAnswer() {
    return lastAnswer;
}

void game::player::setAnswer(std::string &answer) {
    lastAnswer = answer;
}






// Server methods implementation

void game::SessionServer::messaging(std::string messageForClient, int i) {
    std::lock_guard<std::mutex> lock(players[i]->playerMutex);
    if (players[i]->connected) {
        try {
            size_t bytes_written = asio::write(*(players[i]->socket), asio::buffer(messageForClient + "~"));
            if (bytes_written > 0) {
                players[i]->lastActive = std::chrono::steady_clock::now();
            }
        } catch (std::system_error& e) {
            std::cerr << "Error writing to socket: " << e.what() << std::endl;
        }
    }
}

void game::SessionServer::broadcasting(std::string messageForClients) {
    int n = players.size();
    std::cout
    << YELLOW
    <<messageForClients
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
        messaging("Retard", j);
    }
    playersList = std::move(playersListNew);
    broadcasting(playersList);
}

void game::SessionServer::monitor_clients(int clientid) {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        if ((std::chrono::steady_clock::now() - players[clientid]->lastActive) > std::chrono::seconds(5)) {
            std::unique_lock<std::mutex> lock(players[clientid]->playerMutex);
            std::cout << "\n" << RED << players[clientid]->name << " disconnected \n" << RESET;
            players[clientid]->connected = false;
            lock.unlock();
            playersBroadcasting();
            broadcasting("ChatTechnical:" + players[clientid]->name + " left");\
            parent->connectedPlayers--;

            if (!gameStarted) {
                break;
            }
        }
        for (int i = 0; i < players.size(); i++) {
            std::cout
            << PINK
            << "\nplayer" << i << ": " << players[i]->name << " |connection status: " << players[i]->connected
            << RESET <<  std::endl;
        }
    }
}

int game::SessionServer::addPlayer(std::string name) {
    auto p = std::make_shared<player>();
    p->name = std::move(name);
    p->connected = true;
    p->lastActive = std::chrono::steady_clock::now();
    p->ping = "69";
    players.push_back(p);
    return (players.size() - 1);
}

void game::SessionServer::handle_client(tcp::socket socket) {
    std::cout
    << MAGENTA
    << "\nclient handling...\n"
    << RESET ;
    int clientId = -1;
    try {
        while (true) {
            char buffer[1024];
            asio::error_code error;
            tcp::socket& working_socket = (clientId == -1) ? socket : *(players[clientId]->socket);

            size_t length = working_socket.read_some(asio::buffer(buffer), error);
            if (!error) {
                std::string clientMessage(buffer, length);
                if (clientMessage.find("ping") != std::string::npos) {
                    std::lock_guard<std::mutex> lock(players[clientId]->playerMutex);
                    asio::write(working_socket, asio::buffer("pong~"));
                    length = working_socket.read_some(asio::buffer(buffer), error);
                    std::string ping(buffer, length);
                    std::string pingstring = (std::to_string(std::round(std::stof(ping))));
                    players[clientId]->ping = pingstring.substr(0, pingstring.find('.'));

                    players[clientId]->lastActive = std::chrono::steady_clock::now();
                } else if (clientMessage.find("nickname:") != std::string::npos) {
                    clientMessage = clientMessage.substr(9);
                    std::cout
                    << GREEN
                    << "Nickname received: " << clientMessage
                    << RESET << "\n";
                    clientId = addPlayer(clientMessage);
                    players[clientId]->socket.emplace(std::move(socket));
                    playersBroadcasting();
                    broadcasting("ChatTechnical:" + clientMessage + " joined");
                    parent->connectedPlayers++;

                    std::thread monitor_thread(&game::SessionServer::monitor_clients, this, clientId);
                    monitor_thread.detach();

                } else if (clientMessage == "startingGame") {
                    std::cout
                    << GREEN
                    << "Starting Game"
                    << RESET <<  std::endl;
                    std::thread gameStarting_thread(&game::SessionServer::gameStarting, this);
                    gameStarting_thread.detach();
                } else if (clientMessage.find("clientMessage:") != std::string::npos) {
                    broadcasting(clientMessage);
                } else if (clientMessage.find("Answer:") != std::string::npos) {
                    clientMessage = clientMessage.substr(7);
                    handlingPlayerAnswer(clientMessage, clientId); // saving player's last answer, and checking if its right.
                }
                std::cout
                << MAGENTA
                << "data collected\n"
                << clientMessage << "\n"
                << RESET ;
            } else {
                std::cerr << "Error reading message: " << error.message() << " (Connection failed)" << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(11));
                break;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << "\n";
    }
}



void game::SessionServer::gameStarting() {
    int duration = 5;
    for (int i = duration; i > 0; --i) {
        broadcasting("timer:" + std::to_string(i));
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    broadcasting("Started");
    parent->gameCore_instance.launchRound();
}

void game::SessionServer::connecting(tcp::acceptor& acceptor, asio::io_context& io_context) {
    try {
        std::cout
        << GRAY
        << "Waiting for client connection..."
        << RESET << "\n";
        acceptor.async_accept([this, &io_context, &acceptor](const asio::error_code& error, tcp::socket socket) {
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
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

void game::SessionServer::server_starting() {
    system(("chcp "+ std::to_string(CP_UTF8)).c_str());
    try {

        asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 12345));
        std::cout << "Server is running with port: 12345\n";

        std::thread ping_thread(&game::SessionServer::ping_broadcasting, this);
        ping_thread.detach();
        connecting(acceptor, io_context);

        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}
