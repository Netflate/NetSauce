#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <mutex>
#include <functional>
#include <asio.hpp>
#include <iostream>
#include <unordered_set>

using asio::ip::tcp;

class game {
public:
    int connectedPlayers = 0 ;

    class player {
    public:
        std::string name; // nickname
        // related to server side
        bool connected = false;
        float responseTime;
        std::string ping;
        std::chrono::steady_clock::time_point lastActive;
        std::optional<tcp::socket> socket;
        std::mutex playerMutex;


        std::string getAnswer();
        void setAnswer(std::string& answer);
        void setAnswered(bool newStatus);
        bool getAnswered() const;

        int returnScore() const ;
        int& addnReturnScore(const int& toAdd);
    private:
        // directly related to the game
        int score = 0;
        std::string lastAnswer;
        bool answered = false;
    };

    class gameCore {
    public:
        using Signal = std::function<void()>;

        gameCore(game* g) : parent(g), answering(g->connectedPlayers) {};

        struct gameData {
            int id;
            std::string question_type;
            std::string link;
            std::string question_cat;
            std::string name;
            std::string description;
            std::string abbreviation;
        };

        bool getGameEnd() const;
        void setGameEnd(bool);

        int getTimer() const;
        void setTimer(int t);
        float getHowMuchTimeLeft() const;

        std::unordered_set<std::string> getCat() const;
        std::unordered_set<std::string> getType() const;
        void setCat(const std::unordered_set<std::string>& cats);
        void setType(const std::unordered_set<std::string>& types);

        void setWinningScore(int score);
        int getWinningScore() const;

        std::string normalised(const std::string& str);
        bool answer(std::string answer);
        void launchRound();
        gameData GameInfo(std::string line);
        std::string SelectedGame(std::unordered_map<int, std::string>& variantList);
        std::vector<std::string> splitString(const std::string& s, char delim);
        std::unordered_map<int, std::string> getVariantList();
        int getAnswering() const;
        std::string getTrueAnswer() const;


        void setSignal(Signal sig) {
            std::cout << "Signal handler set." << std::endl;
            signal = sig;
        }
        void triggerSignal() {

            if (signal) {
                signal();
            }
        }

    private:
        game* parent;

        float timeLeft ;

        std::string trueAnswer;
        std::string trueAnswerAbbreviation;

        int answering; // by default will be connected players number, decreasing with players answering right down to 0
        bool end;
        int timer;
        int winningScore = 150;

        std::unordered_set<std::string> categories;
        std::unordered_set<std::string> questions_type;
        Signal signal;
    };

    class SessionServer {
    private:
        game* parent;
        gameCore& gameplay; // making object gameplay a part of sessionServer class

        std::mutex well_well_well;
        std::string playersList = "";
        std::vector<std::shared_ptr<player>> players;
        float firstPlayerResponseTime;


        void signalHandler();

    public:
        SessionServer(game* g) : parent(g), gameplay(g->gameCore_instance) {
            gameplay.setSignal([this]() { signalHandler(); });
            std::cout << "SessionServer constructed, signal handler connected." << std::endl;
        }

        std::string winningPlayer() const;
        void handlingPlayerAnswer(std::string& answer, const int& clientId);
        void messaging(const std::string messageForClient, int i);
        void broadcasting(const std::string messageForClients);
        void ping_broadcasting();
        void playersBroadcasting();
        void handle_client(tcp::socket socket);
        void monitor_clients(int clientid);
        int addPlayer(std::string name);
        void connecting(tcp::acceptor& acceptor, asio::io_context& io_context);
        void server_starting();
        void gameStarting();
    };

    game() : gameCore_instance(this), server_instance(this) {};
    gameCore gameCore_instance;
    SessionServer server_instance;

};

#endif // SERVER_H
