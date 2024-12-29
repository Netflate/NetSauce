#include "Game.h"
#include "gameMechanic.h"

#include <iostream>
#include <ctime>
#include <iostream>
#include <fcntl.h>
#include <winsock2.h>
#include <asio.hpp>
using asio::ip::tcp;


void Players_broadcasting() {
    int n = players.size();
    std::string playersList = "playersList:";
    for (int i = 0; i < n; i++) {
        if (players[i].connected == true) {
            playersList +=players[i].name + "#";
        }
    }
    std::cout << playersList << std::endl;
    for (int i = 0; i < n; i++ ) {
        if (players[i].connected == true) {
            std::cout << "TESTinFor" << std::endl;
            try {
                size_t bytes_written = asio::write(*players[i].socket, asio::buffer(playersList));
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
        if ((std::chrono::steady_clock::now() - players[clientid].lastActive) > std::chrono::seconds(5)) {
            std::cout << "\n" << players[clientid].name << " disconnected ig";
            players[clientid].connected = false;
            Players_broadcasting();
            if (!gameStarted) {
                break;
            }
        }
        for (int i = 0; i < players.size(); i++) {
            std::cout << "\nplayer" << i << ": "<< players[i].name << " |connection status: " << players[i].connected  << std::endl;
        }

    }
}

void handle_client(tcp::socket socket) {
    std::cout << "\nclient hanlding...\n";
    int clientId = -1; //  unknown, untill function addplayers
    try {
        while (true) {
            char buffer[1024];
            asio::error_code error;
            size_t length = socket.read_some(asio::buffer(buffer), error);
            if (!error) {
                std::string clientMessage (buffer, length);
                if (clientMessage.find("ping") != std::string::npos) {
                    std::cout << "client id: " << clientId << std::endl; ;
                    std::cout << "ping \n" ;
                    asio::write(socket, asio::buffer("pong"));
                    size_t length = socket.read_some(asio::buffer(buffer), error);
                    std::string ping (buffer, length);

                    // Выводим время в секундах с начала эпохи steady_clock
                    std::cout << "last time active: "
                              << std::chrono::duration_cast<std::chrono::seconds>(
                                     players[clientId].lastActive.time_since_epoch()
                                 ).count()
                              << " seconds since steady_clock epoch" << std::endl;

                    // Обновляем время
                    players[clientId].lastActive = std::chrono::steady_clock::now();

                    // Снова выводим время в секундах
                    std::cout << "last time active: "
                              << std::chrono::duration_cast<std::chrono::seconds>(
                                     players[clientId].lastActive.time_since_epoch()
                                 ).count()
                              << " seconds since steady_clock epoch" << std::endl;

                    std::cout << ping << std::endl;

                }
                else if (clientMessage.find("nickname:") != std::string::npos) {
                    clientMessage = clientMessage.substr(9);
                    std::cout << "Nickname recieved: " << std::string(clientMessage) << "\n";
                    clientId = addPlayer(clientMessage);
                    players[clientId].socket = &socket;
                    Players_broadcasting();
                    std::thread monitor_thread(monitor_clients, clientId);
                    monitor_thread.detach();
                }
                //lobby();
                std::cout << "\ndata collected\n" ;
            } else {

                std::cerr << "Error reading message: " << error.message() << " (Connection failed)" << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(11));
                break;
            }





        }
    } catch (const std::exception& e) {
        std::cout << "pizzzzzzzzzdez"<< std::endl;
        std::cerr << "Exception caught: " << e.what() << "\n";
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
        std::cout << "pizdez";
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

