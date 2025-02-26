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
    int skippingPlayers = 0;
    void reset() {
        connectedPlayers = 0;
        gameCore_instance = gameCore(this);

        // reconnecting signals
        server_instance.reconnectSignals();

        // resetting some server information
        server_instance.resetSession();
    }
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

        void setScore(const int sc);
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
        using ParamSignal = std::function<void(const std::string&)>;

        gameCore(game* g) : parent(g), answering(g->connectedPlayers) {};

        struct gameData {
            int id;
            std::string question_type;
            std::string link;
            std::string question_cat;
            std::string name;
            std::string description;
            std::string abbreviation;

            std::string pause;   // if question_type == "video"
            std::string show;

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
        std::string questionInfo;

        // First signal
        void setSignal(Signal sig) {
            std::cout << "Signal handler set." << std::endl;
            signal = sig;
        }
        void triggerSignal() {

            if (signal) {
                signal();
            }
        }
        // second one
        void setParamSignal(ParamSignal sig) {
            paramSignal = sig;
        }
        void triggerParameterSignal(const std::string& stringToSend) {
            if (paramSignal) {
                paramSignal(stringToSend);
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
        ParamSignal paramSignal;
    };

    class SessionServer {
    private:
        game* parent;
        gameCore& gameplay; // making object gameplay a part of sessionServer class
        int leader_player_id;

        bool gameStarted = false;
        std::mutex well_well_well;
        std::string playersList = "";
        std::unordered_set<std::string> used_nicknames;
        std::vector<std::shared_ptr<player>> players;
        float firstPlayerResponseTime;


        std::string questionCatsList();
        void signalHandler();
        void paramSignalHandler(const std::string& stringToSend);

    public:
        SessionServer(game* g) : parent(g), gameplay(g->gameCore_instance) {
            reconnectSignals();
            std::cout << "SessionServer constructed, signal handlers connected." << std::endl;
        }
        // reconnecting signals, important for server resetting
        void reconnectSignals() {
            gameplay.setSignal([this]() { signalHandler(); });
            gameplay.setParamSignal([this](const std::string& m) {
                paramSignalHandler(m);
            });
        }

        void resetSession() {
            players.clear();
            playersList = "";
            firstPlayerResponseTime = 0;
            gameStarted = false;
            // Добавьте сброс других необходимых переменных
        }
        void players_game_information_reset();
        std::unordered_set<std::string> parseCategories(const std::string& categoriesStr, char delimiter);
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
