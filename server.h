#ifndef SERVER_H
#define SERVER_H

#include <SFML/Network.hpp>
#include <QMap>
#include <QObject>
#include <sstream>
#include "initdb.h"

class initdb;

class Server : public QObject
{

Q_OBJECT

private:
    // db wrapper
    initdb *db;

    // key usernames to client sockets
    std::map<std::string, sf::TcpSocket*> clientMap;

    // store socket connections
    sf::SocketSelector selector;

    // The port number for the server
    unsigned int port;

    // The TCP Listener object
    sf::TcpListener *listener;

    // store temporary initial client connection before auth.
    sf::TcpSocket *tempClient;

    void connectSignals();

    // router method to perform client request
    void handleClientRequest(sf::Packet*);

    bool testDbConnection();

public:
    Server(unsigned int portNumber);

    void start(); // bootstrap the server
    bool send(sf::TcpSocket socket, sf::Packet packet);
    bool close();
    void waitForClientDataLoop();

public slots:
    // fires when the initdb signals with a login result
    void onSendLoginResult(std::string, std::string, int=0, int=1);

    // fires when the initdb signals with a register result
    void onSendRegisterResult(std::string, std::string);

    // fires when a client/admin request games scores for a certain user for a certain game
    void onSendPlayerStats(std::string, std::list<double>*);

    void onSendPlayerNames(std::string, std::list<std::string>);
    void onSendAllGameScores(std::string, std::string,std::list<double>*,std::list<double>*,std::list<double>*,std::list<double>*,std::list<std::string>*,std::list<std::string>*,std::list<std::string>*);

signals:
    // pings the db, checks for healthy connection
    void connectedToDb(bool);

    // fires when the listener accepts a new client
    void newClientAccepted(sf::TcpSocket *socket);

    // fires when a socket receives data
    void dataReceived(sf::Packet *packet);

    // new user
    void userRegister(std::string, std::string);

    // user login
    void userLogin(std::string, std::string);

    // used to shortcircuit a login attempt incase a user attempts to login twice
    void sendLoginResult(std::string, std::string, int, int);

    // send new player scores to db handler
    void sendScores(std::string, std::string, std::list<double>*);

    // update the player's progress
    void updateLevel(std::string,double);

    // signal the db to query for a user's statistics for a specific game
    void queryGameScores(std::string, std::string);

    // get names of players from db
    void getPlayerNames(std::string);

    // delete user and score footprints from all tables
    void deleteUser(std::string);

    // get a list of all scores
    void getAllGameScores(std::string, std::string);

};

#endif // SERVER_H
