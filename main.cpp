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
    srand(time(nullptr));
    server_starting();

    return 0;
}
