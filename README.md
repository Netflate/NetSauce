# NetSauce game server

## Overview
This repository contains the server-side component of a multiplayer guessing game. The game is built around a question-and-answer format where players try to identify games, movies, or other subjects based on images, videos, or text clues.
It was pretty much copied from a web multiplayer game [popSauce](https://jklm.fun) 
## Project Structure
The server is built with C++ and is organized into several key components:

- **Game Core (`Game.cpp`)**: Handles the game logic, including question selection, answer validation, and round timing.
- **Server (`server.cpp`, `server.h`)**: Manages client connections, player sessions, and communication between clients and the game core.
- **Main (`main.cpp`)**: Entry point that initializes the server and implements automatic server recovery in case of crashes.

## Features

### Game Mechanics
- **Round-based gameplay**: Each round presents players with a question to answer
- **Multiple question types**: Support for image, gif, and video-based questions
- **Score system**: Points awarded based on answer speed and accuracy
- **Time-based rounds**: Each round has a configurable timer
- **Skip voting**: Players can vote to skip difficult questions
- **Winner determination**: Players compete to reach a configurable winning score

### Networking
- **TCP/IP connections**: Uses ASIO library for asynchronous network operations
- **String-based protocol**: Custom string format for client-server communication
- **Session management**: Tracks player connections, disconnections, and timeouts
- **Broadcasting**: Sends game updates to all connected clients
- **Real-time updates**: Provides countdown timers and immediate answer feedback

### Player Management
- **Unique nicknames**: Prevents duplicate player names
- **Connection monitoring**: Detects and handles disconnected players
- **Leader system**: First connected player becomes the game leader with additional privileges
- **Chat functionality**: Basic chat system for player communication

## Technology Stack
- **C++**: Core programming language
- **ASIO**: Network programming library (non-Boost version)
- **Standard Library**: Extensive use of STL containers and algorithms
- **Multi-threading**: Thread management for handling multiple clients simultaneously
- **Signal-slot pattern**: For communication between game components

## Technical Details

### Key Functions and Implementation

#### Game Logic (`Game.cpp`)
- **`launchRound()`**: The core game loop function that manages the entire round lifecycle:
  - Selects random questions from the question database
  - Controls round timing with a countdown timer
  - Monitors player answers and voting
  - Updates game state and broadcasts to clients
  - Handles end-of-round activities including score calculation

- **`answer(std::string answer)`**: Sophisticated answer validation that:
  - Checks for exact matches with the correct answer
  - Supports abbreviation matching
  - Implements fuzzy matching with the `normalised()` function
  - Updates player answering status

- **`normalised(const std::string& str)`**: String normalization function that:
  - Converts text to lowercase
  - Removes common articles like "the"
  - Strips punctuation and special characters
  - Creates a consistent format for answer comparison

- **`SelectedGame(std::unordered_map<int, std::string>& variantList)`**: Question selection algorithm that:
  - Uses Mersenne Twister random number generator for true randomness
  - Selects questions from available pool
  - Removes selected questions to prevent repetition

#### Server Implementation (`server.cpp`)
- **`handle_client(tcp::socket socket)`**: Complex client handler that:
  - Processes all client messages using a command dispatch system
  - Manages player authentication and nickname verification
  - Routes game commands to appropriate handlers
  - Maintains client connection state

- **`handlingPlayerAnswer(std::string &answer, const int &clientId)`**: Answer processing system that:
  - Validates player answers against correct answer
  - Calculates scores based on response time
  - Handles skip voting logic
  - Broadcasts results to all clients

- **`monitor_clients(int clientid)`**: Client monitoring thread that:
  - Detects inactive or disconnected clients
  - Updates server state when clients disconnect
  - Cleans up resources for disconnected clients
  - Broadcasts connection status changes

- **`broadcasting(std::string messageForClients)`**: Message distribution system that:
  - Sends updates to all connected clients
  - Handles client-specific error conditions
  - Logs communication for debugging

### Communication Protocol
The server and client communicate using a custom string-based protocol rather than a structured format like JSON. This was an educational choice, and in a production environment, a more robust format like JSON would be preferable. Messages use delimiters like ":" and "#" to separate command types and parameters.

Example message formats:
- `"questionInfo:image:link:http://example.com/image.jpg:hint:This game was released in 2020"`
- `"ParsePlayerData:PlayerName#AnswerGiven#100"`
- `"timer:5"`

This approach was maintained throughout development as switching to JSON would have required significant refactoring. For future projects, implementing a structured data format from the beginning would be more maintainable.

### Networking Architecture
The server uses a multi-threaded approach where each client connection is handled in a separate thread. The ASIO library provides asynchronous I/O operations that allow for efficient handling of multiple connections.

### Error Handling
The server includes basic error handling to recover from:
- Network disconnections
- Invalid client messages
- Server crashes (automatic restart)

## Disclaimer
This is my first project in C++ and server development. I built this while learning both technologies, so the code is not optimized for production use or intended for third-party implementation. It's primarily a demonstration of my skills and learning progress.

The code contains areas that could be improved:
- Error handling could be more robust
- Code organization could be more modular
- Some performance optimizations could be implemented
- Security considerations should be addressed for real-world use
- The string-based communication protocol could be replaced with JSON or another structured format

## Client Repository
The client-side component of this game is available in a separate repository at [INSERT CLIENT REPO LINK HERE].

## Setup and Usage
1. Compile the server using a C++ compiler with C++17 support
2. Run the executable
3. The server will start on port 12345
4. Connect with the client application to play

## Future Possible Improvements
- Enhanced error handling
- Better code organization
- Performance optimizations
- Additional game modes
- Improved security measures
- Replace string-based messaging with JSON or Protocol Buffers
