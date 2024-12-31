#include "Game.h"
#include "gameMechanic.h"

#include <iostream>
#include <ctime>
#include <iostream>
#include <fcntl.h>
#include <winsock2.h>
#include <asio.hpp>
#include <mutex>

using asio::ip::tcp;
std::mutex well_well_well;
void Players_broadcasting() {
    std::lock_guard<std::mutex> lock(well_well_well);
    int n = players.size();
    std::string playersList = "playersList:";
    for (int i = 0; i < n; i++) {
        std::lock_guard<std::mutex> lock(players[i]->playerMutex);
        if (players[i]->connected == true) {
            playersList += players[i]->name + "#";
        }
    }
    std::cout << playersList << std::endl;
    for (int i = 0; i < n; i++) {
        std::lock_guard<std::mutex> lock(players[i]->playerMutex);
        if (players[i]->connected == true) {
            std::cout << "TESTinFor" << std::endl;
            try {
                size_t bytes_written = asio::write(*(players[i]->socket), asio::buffer(playersList));
                std::cout << "Bytes written: " << bytes_written << std::endl;
            } catch (std::system_error& e) {
                std::cerr << "Error writing to socket: " << e.what() << std::endl;
            }
        }
    }
    std::cout << "TEST" << std::endl;
}

void monitor_clients(int clientid) {
    std::cout << "client id: " << clientid << std::endl;
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        if ((std::chrono::steady_clock::now() - players[clientid]->lastActive) > std::chrono::seconds(5)) {
            std::unique_lock<std::mutex> lock(players[clientid]->playerMutex);
            std::cout << "\n" << players[clientid]->name << " disconnected ig";
            players[clientid]->connected = false;
            lock.unlock();
            Players_broadcasting();

            if (!gameStarted) {
                break;
            }
        }
        for (int i = 0; i < players.size(); i++) {
            std::cout << "\nplayer" << i << ": " << players[i]->name << " |connection status: " << players[i]->connected << std::endl;
        }
    }
}

void handle_client(tcp::socket socket) {
    std::cout << "\nclient hanlding...\n";
    std::cout << "10\n";
    int clientId = -1;
    try {
        while (true) {
            char buffer[1024];
            asio::error_code error;
            // Before clientId is set, it uses the originale one
            tcp::socket& working_socket = (clientId == -1) ? socket : *(players[clientId]->socket);

            size_t length = working_socket.read_some(asio::buffer(buffer), error);
            std::cout << "11\n";
            if (!error) {
                std::string clientMessage(buffer, length);
                if (clientMessage.find("ping") != std::string::npos) {
                    std::lock_guard<std::mutex> lock(players[clientId]->playerMutex);
                    std::cout << "client id: " << clientId << std::endl;
                    std::cout << "ping \n";
                    asio::write(working_socket, asio::buffer("pong"));
                    length = working_socket.read_some(asio::buffer(buffer), error);
                    std::string ping(buffer, length);

                    std::cout << "last time active: "
                              << std::chrono::duration_cast<std::chrono::seconds>(
                                     players[clientId]->lastActive.time_since_epoch()
                                 ).count()
                              << " seconds since steady_clock epoch" << std::endl;

                    players[clientId]->lastActive = std::chrono::steady_clock::now();

                    std::cout << "last time active: "
                              << std::chrono::duration_cast<std::chrono::seconds>(
                                     players[clientId]->lastActive.time_since_epoch()
                                 ).count()
                              << " seconds since steady_clock epoch" << std::endl;

                    std::cout << ping << std::endl;
                }
                else if (clientMessage.find("nickname:") != std::string::npos) {
                    clientMessage = clientMessage.substr(9);
                    std::cout << "Nickname recieved: " << std::string(clientMessage) << "\n";
                    clientId = addPlayer(clientMessage);
                    players[clientId]->socket.emplace(std::move(socket)); // After this, socket is mooooooooooooooved i like to move it move it, i like to, move it
                    Players_broadcasting();
                    std::thread monitor_thread(monitor_clients, clientId);
                    monitor_thread.detach();
                }
                std::cout << "\ndata collected\n";
                std::cout << "12\n";
            } else {
                std::cerr << "Error reading message: " << error.message() << " (Connection failed)" << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(11));
                break;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << "\n";
        std::cout << "13\n";
    }
}

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

int main() {
    srand(time(nullptr));
    try {
        asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 12345));
        std::cout << "Server is running with port: 12345\n";
        connecting(acceptor, io_context);

        io_context.run(); // Продолжаем выполнение io_context в основном потоке
    }
    catch (std::exception& e) {
        std::cout << "pizdez2";
        std::cerr << "Error: " << e.what() << "\n";
    }

    return 0;
}
