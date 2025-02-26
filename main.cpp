#include "Game.h"
#include "gameMechanic.h"
#include "server.h"
#include <stdio.h>

#include <iostream>
#include <ctime>
#include <iostream>
#include <fcntl.h>
#include <winsock2.h>
#include <asio.hpp>
#include <cmath>
#include <mutex>



int main() {
    while (true) {  // Бесконечный цикл для поддержания работы сервера
        try {
            game g;
            g.server_instance.server_starting();
        }
        catch (const std::exception& e) {
            std::cerr << "Server crashed: " << e.what() << "\n";
            std::cerr << "Restarting server...\n";
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    return 0;
}