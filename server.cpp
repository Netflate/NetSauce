#include "server.h"
#include "Game.h"
#include "gameMechanic.h"

#include <iostream>
#include <thread>
#include <cmath>
#include <asio.hpp>
#include <mutex>

using asio::ip::tcp;

// Global variables
std::mutex well_well_well;
std::string playersList = "";


/******************************************************
                Server to Client stuff
******************************************************/

// Sending data to one single client //

void messaging(std::string messageForClient, int i) {
    std::lock_guard<std::mutex> lock(players[i]->playerMutex);
    if (players[i]->connected == true) {                         // checking if the client is connected
        try {
            size_t bytes_written = asio::write(*(players[i]->socket), asio::buffer(messageForClient + "~")); // sending
            if (bytes_written > 0) {
                players[i]->lastActive = std::chrono::steady_clock::now();        // updating last active time, just in case
            }
        } catch (std::system_error& e) {
            std::cerr << "Error writing to socket: " << e.what() << std::endl;
        }
    }
}

// Sending data to all connected clients //

void broadcasting(std::string messageForClients) {
    int n = players.size();  // clients list
    std::cout << messageForClients << "\n"; // printing message
    for (int i = 0; i < n; i++) {
        messaging(messageForClients, i);
    }
}


// Ping broadcasting to all clients  //

void ping_broadcasting () {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(4));
        int n = players.size();
        if (n > 0) {
            std::string pingList = "playersPing:";
            for (int i = 0; i < n; i++) {
                std::lock_guard<std::mutex> lock(players[i]->playerMutex);
                if (players[i]->connected == true) {
                    pingList += players[i]->ping + "#";
                }
            }
            broadcasting(pingList);
        }
    }
}

// players list broadcasting to all clients //

void Players_broadcasting() {
    std::lock_guard<std::mutex> lock(well_well_well);
    int n = players.size();
    std::string playersListNew = "playersList:";
    int j = 0 ;
    for (int i = 0; i < n; i++) {
        std::lock_guard<std::mutex> lock(players[i]->playerMutex);
        if (players[i]->connected) {
            playersListNew += players[i]->name + "#";
            if (j==0) j = i;;

        }
    }
    if (playersListNew.substr(0, playersListNew.find("#")) != playersList.substr(0, playersList.find("#"))   ) {
        messaging("Retard", j);
    }
    playersList = std::move(playersListNew);
    broadcasting(playersList);
}


/******************************************************
                Client to Server functions
******************************************************/


// A function made for monitoring clients online status; if client's last time activity exceeds 5 seconds
//                                                       his "connected" status changes to offline
//                                                       and then all the clients get the message
//                                                       that one player has disconnected via broadcasting function

void monitor_clients(int clientid) {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        if ((std::chrono::steady_clock::now() - players[clientid]->lastActive) > std::chrono::seconds(5)) {
            std::unique_lock<std::mutex> lock(players[clientid]->playerMutex);
            std::cout << "\n" << players[clientid]->name << " disconnected ig";
            players[clientid]->connected = false;
            lock.unlock();
            Players_broadcasting();                     // updating players list
            broadcasting("ChatTechnical:"+ players[clientid]->name+ " left"); // sending disconnection message for all clients

            if (!gameStarted) {
                break;
            }
        }
        for (int i = 0; i < players.size(); i++) {
            std::cout << "\nplayer" << i << ": " << players[i]->name << " |connection status: " << players[i]->connected << std::endl;
        }
    }
}

// endless function made for getting messages from clients, and then decide what to do with it
// This function works individually for each client

void handle_client(tcp::socket socket) {
    std::cout << "\nclient hanlding...\n";
    int clientId = -1;
    try {
        while (true) {
            char buffer[1024];
            asio::error_code error;
            tcp::socket& working_socket = (clientId == -1) ? socket : *(players[clientId]->socket);   // Before clientId is set, it uses the original one (socket)

            size_t length = working_socket.read_some(asio::buffer(buffer), error);
            if (!error) {
                std::string clientMessage(buffer, length);
                if (clientMessage.find("ping") != std::string::npos) {                       // PING HANDLING
                    std::lock_guard<std::mutex> lock(players[clientId]->playerMutex);
                    std::cout << "client id: " << clientId << std::endl;
                    std::cout << "ping \n";
                    asio::write(working_socket, asio::buffer("pong~"));
                    length = working_socket.read_some(asio::buffer(buffer), error);
                    std::string ping(buffer, length);
                    std::string pingstring = (std::to_string(std::round(std::stof(ping))));
                    players[clientId]->ping = pingstring.substr(0, pingstring.find('.'));

                    players[clientId]->lastActive = std::chrono::steady_clock::now();
                    std::cout << ping << std::endl;
                }
                else if (clientMessage.find("nickname:") != std::string::npos) {            // CLIENT'S NICKNAME
                    clientMessage = clientMessage.substr(9);
                    std::cout << "Nickname recieved: " << std::string(clientMessage) << "\n";
                    clientId = addPlayer(clientMessage);
                    players[clientId]->socket.emplace(std::move(socket)); // After this, socket is moved, so socket var is no longer accessible
                    Players_broadcasting();
                    broadcasting("ChatTechnical:"+ clientMessage + " joined");
                    std::thread monitor_thread(monitor_clients, clientId);
                    monitor_thread.detach();

                }
                else if (clientMessage == "StartingGame") {                                    // CLIENT HOST STARTED THE GAME
                    std::cout << "Starting Game" << std::endl;
                    std::thread gameStarting_thread(gameStarting);
                    gameStarting_thread.detach();
                }
                else if (clientMessage.find("clientMessage:") != std::string::npos) {       // CLIENT's MESSAGE (CHAT)
                    broadcasting(clientMessage);
                }
                std::cout << "\ndata collected\n";
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



/*****************************************************************************************
                OTHER FUNCTIONS (connecting and main, at least for now)
*****************************************************************************************/


void connecting(tcp::acceptor& acceptor, asio::io_context& io_context) {
    try {
        std::cout << "Waiting for client connection...\n";
        acceptor.async_accept([&io_context, &acceptor](const asio::error_code& error, tcp::socket socket) {
            if (!error) {
                std::cout << "Client connected: " << socket.remote_endpoint() << "\n";
                std::thread(&handle_client, std::move(socket)).detach();
                connecting(acceptor, io_context);
            } else {
                std::cout << "Error accepting connection: " << error.message() << "\n";
            }
        });

        io_context.run();
    }
    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}

void server_starting() {
    try {
        asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 12345));
        std::cout << "Server is running with port: 12345\n";
        std::thread ping_thread(ping_broadcasting);
        ping_thread.detach();
        connecting(acceptor, io_context);

        io_context.run(); // Продолжаем выполнение io_context в основном потоке
    }
    catch (std::exception& e) {
        std::cout << "pizdez2";
        std::cerr << "Error: " << e.what() << "\n";
    }
}

