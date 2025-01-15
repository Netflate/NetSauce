#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <mutex>
#include <asio.hpp>

using asio::ip::tcp;

// Function declarations
void messaging(std::string messageForClient, int i);
void broadcasting(std::string messageForClients);
void ping_broadcasting();
void Players_broadcasting();
void gameStarting();
void handle_client(tcp::socket socket);
void connecting(tcp::acceptor& acceptor, asio::io_context& io_context);
void server_starting();

// Global variables declarations
extern std::mutex well_well_well;
extern std::string playersList;

#endif // SERVER_H