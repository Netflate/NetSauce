cmake_minimum_required(VERSION 3.30) # Версия CMake должна быть >= 3.16
project(NetSauce)

# Устанавливаем стандарт C++
set(CMAKE_CXX_STANDARD 23)
# Указываем путь к заголовочным файлам Asio

include_directories(C:/Games/asio-master/asio/include)
# Создаём исполняемый файл
add_executable(NetSauce
        main.cpp
        Game.cpp
        server.cpp
        )

# Подключаем библиотеку Windows Sockets
target_link_libraries(NetSauce PRIVATE ws2_32 mswsock)

