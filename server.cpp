#include "server.h"

Server::Server(unsigned int portNumber)
{
    this->port = portNumber;
    start();
}

/*
 * Bootstrap the server and open a tcp listener for client connections.
 */
void Server::start()
{
    // initdb instance must have 'this' in order to set up bi-directional signal binding
    this->db = new initdb(0, this);

    if (!testDbConnection())
    {
        return;
    }

    // connect server to db access wrapper
    connectSignals();

    this->listener = new sf::TcpListener;

    // bind the listener to a port
    if (this->listener->listen(this->port) != sf::Socket::Done)
    {
        std::cout << "Failed to bind to port: " << this->port << std::endl;
    }

    std::cout << "Successfully binded to port: " << this->port << std::endl;

    // accept a new connection
    std::cout << "Waiting for clients to connect" << std::endl;

    this->listener->setBlocking(false);

    this->selector.add(*this->listener);

    // polling for new client connections
    while(true)
    {
        // waits for a socket to be ready
        if (this->selector.wait())
        {
            // this block handles new client connections
            if (this->selector.isReady(*this->listener))
            {
                // empty placeholder socket used hold the connection details of any connected client
                sf::TcpSocket *client = new sf::TcpSocket;

                // if the listener successfully accepts a new client connection
                if (this->listener->accept(*client) == sf::Socket::Done)
                {
                    // store client socket connection
                    this->selector.add(*client);

                    sf::Packet* packet = new sf::Packet;

                    if(client->receive(*packet) == sf::Socket::Done)
                    {
                        this->tempClient = *&client;
                        handleClientRequest(packet);
                    }
                }
                else
                {
                    delete client;
                }
            }
            // this blocks handles client-server interactions via previously set up sockets
            else
            {
                // The listener socket is not ready, test all other sockets (the clients)
                for (std::map<std::string, sf::TcpSocket*>::iterator it = this->clientMap.begin(); it != this->clientMap.end(); ++it)
                {
                    sf::TcpSocket& client = *it->second;

                    if (this->clientMap.size() == 0) // if there are no connected clients
                    {
                        break;
                    }

                    if (this->selector.isReady(client))
                    {
                        // The client has sent some data, we can receive it
                        sf::Packet* packet = new sf::Packet;

                        sf::TcpSocket::Status status = client.receive(*packet);

                        if (status == sf::Socket::Done)
                        {
                            handleClientRequest(packet);
                        }
                        else if (status == sf::Socket::Disconnected)
                        {
                            for (auto& currentClient : this->clientMap)
                            {
                                if (*&currentClient.second == &client)
                                {
                                    this->clientMap.erase(currentClient.first);
                                }
                            }
                            std::cout << "client " << client.getRemoteAddress() << " has disconnected" << std::endl;
                        }
                    }
                }
            }
        }
    }
}

/*
 * Route client response packets to the correct server/db action.
 */
void Server::handleClientRequest(sf::Packet* response)
{
    std::string action, username;
    *response >> action >> username;

    if (action == "Login")
    {
        if (this->clientMap[username])
        {
           emit sendLoginResult(username, "USER_ALREADY_LOGGED_IN", 0, 1);
        }
        else
        {
            this->clientMap[username] = this->tempClient;
            std::cout << "client: " << this->tempClient->getRemoteAddress() << " connected!" << std::endl;
            std::string password;
            *response >> password;
            emit userLogin(username, password);
        }
    }
    else if (action == "Register")
    {
        std::string password;
        *response >> password;
        this->clientMap[username] = this->tempClient;
        emit userRegister(username, password);
    }
    else if (action == "Cowboy_Scores" || action == "Fighting_Scores" || action == "Racing_Scores")
    {
        emit queryGameScores(action, username);
    }
    else if (action == "SEND_COWBOY_SCORES" || action == "SEND_RACING_SCORES" || action == "SEND_FIGHTING_SCORES") {
        std::string table;

        if (action == "SEND_COWBOY_SCORES") table = "COWBOY_SCORES";
        else if (action == "SEND_RACING_SCORES") table = "RACING_SCORES";
        else if (action == "SEND_FIGHTING_SCORES") table = "FIGHTING_SCORES";

        double score, wpm, wordsTyped, accuracy, difficulty;
        *response >> score >> wpm >> wordsTyped >> accuracy >> difficulty;
        emit sendScores(username, table, new std::list<double> {score, wpm, wordsTyped, accuracy, difficulty});
    }
    else if (action == "Update_Level")
    {
        double level;
        *response >> level;
        emit updateLevel(username, level);
    }
    else if (action == "Get_Player_Names")
    {
        emit getPlayerNames(username);
    }
    else if (action == "Delete_User")
    {
        emit deleteUser(username);
    }
    else if (action == "Get_All_Scores")
    {
        std::string clientName;
        *response >> clientName;
        emit getAllGameScores(username, clientName);
    }
    else
    {
        std::cout << "client request not recognized" << std::endl;
    }
}

/*
 * Test for healthy connection to database. Allow for repeated attempts at connecting.
 */
bool Server::testDbConnection()
{
    // test db connection
    bool isConnected = db->testConnection();

    if(!isConnected)
    {
        do
        {
            std::cout << "Connection to db could not be established. Try again? (y/n)" << std::endl;
            std::string result;
            std::cin >> result;
            if (result == "y" || result == "Y" || result == "yes" || result == "YES" || result == "Yes")
            {
                isConnected = db->testConnection();
            }
            else if (result == "n" || result == "N" || result == "no" || result == "NO" || result == "No")
            {
                std::cout << "No connection to db could be made. Turning off now." << std::endl;
                return false;
            }
            else
            {
                std::cout << "Input not recognized. Please try again." << std::endl;
            }
        } while (!isConnected);
    }
    return isConnected;
}

bool Server::send(sf::TcpSocket socket, sf::Packet packet)
{
    socket.send(packet);
    return socket.send(packet) == sf::Socket::Done;
}

/*
 * Establish the connection between the server and the database wrapper.
 */
void Server::connectSignals()
{
    QObject::connect(this, SIGNAL(userLogin(std::string,std::string)), this->db, SLOT(onUserLogin(std::string,std::string)));
    QObject::connect(this, SIGNAL(userRegister(std::string,std::string)), this->db, SLOT(onUserRegister(std::string,std::string)));
    QObject::connect(this, SIGNAL(sendLoginResult(std::string, std::string, int, int)), this, SLOT(onSendLoginResult(std::string,std::string,int, int)));
    QObject::connect(this, SIGNAL(sendScores(std::string,std::string,std::list<double>*)), this->db, SLOT(onSendScores(std::string,std::string,std::list<double>*)));
    QObject::connect(this, SIGNAL(updateLevel(std::string,double)), this->db, SLOT(onUpdateLevel(std::string,double)));
    QObject::connect(this, SIGNAL(getPlayerNames(std::string)), this->db, SLOT(onGetPlayerNames(std::string)));
    QObject::connect(this, SIGNAL(getAllGameScores(std::string,std::string)), this->db, SLOT(onGetAllGameScores(std::string,std::string)));
    QObject::connect(this, SIGNAL(deleteUser(std::string)), this->db, SLOT(onDeleteUser(std::string)));
}

//-----------------------------------------------------------------------------------
// SLOTS
//-----------------------------------------------------------------------------------

/*
 * Slot method that fires when the initdb instance sends a signal indicating the status of an attempted login
 */
void Server::onSendLoginResult(std::string username, std::string result, int isAdmin, int highestLevel)
{
    sf::Packet response;

    // prepare login results in packet
    response << username;
    response << result;
    response << isAdmin;
    response << highestLevel;

    if (result == "WRONG_PASSWORD")
    {
        for (auto& currentClient : this->clientMap)
        {
            if (*&currentClient.second == *&this->tempClient)
            {
                this->clientMap.erase(currentClient.first);
            }
        }
        this->tempClient->send(response);
    }
    else if (result == "USER_ALREADY_LOGGED_IN")
    {
        this->tempClient->send(response);
    }
    else
    {
        // send login attempt results back to client
        this->clientMap[username]->send(response);
    }
}

void Server::onSendRegisterResult(std::string username, std::string result)
{
    sf::Packet response;

    // prepare login results in packet
    response << username;
    response << result;

    // send login attempt results back to client
    this->clientMap[username]->send(response);
}

/*
 * Retrieve and handle game scores sent by initdb.
 */
void Server::onSendPlayerStats(std::string username, std::list<double>* scores)
{
    sf::Packet stats;

    for (std::list<double>::iterator it = scores->begin(); it != scores->end(); ++it)
    {
        stats << *it;
    }

    this->clientMap[username]->send(stats);
}

void Server::onSendPlayerNames(std::string username, std::list<std::string> names)
{
    sf::Packet namePacket;

    for (std::list<std::string>::iterator it = names.begin(); it != names.end(); ++it)
    {
        namePacket << *it;
    }

    this->clientMap[username]->send(namePacket);
}

void Server::onSendAllGameScores(std::string username,std::string clientName,std::list<double>* cowboyScores,std::list<double>* racingScores,std::list<double>* fightingScores,std::list<double>* averageScores,std::list<std::string>* cowboyStamps,std::list<std::string>* racingStamps,std::list<std::string>* fightingStamps)
{
    // generate html to send back to client
    std::string html;

    html = "<html>\n";

    // styles
    html += "<style>";

    html += "table.contentTable{";
    html += "   border-collapse: collapse;";
    html += "}\n";
    html += "table.contentTable td, table.contentTable th{";
    html += "   font-family: \"Calibri\";";
    html += "   border: 1px solid #b7b8b6;";
    html += "   padding: 5px;";
    html += "}\n";
    html += "table.contentTable th, .contentTableHeader{";
    html += "   background-color: #47b54a;";
    html += "}\n";
    html += "table.contentTable th, .contentTableHeader{";
    html += "   background-color: #47b54a;";
    html += "   color: white;";
    html += "}\n";
    html += "table.contentTable tr:nth-child(even){";
    html += "   background-color: #d6d6d6;";
    html += "}\n";
    html += "table.contentTable tr:nth-child(odd){";
    html += "   background-color: white;";
    html += "}\n";

    //html += "table, th, td {";
    //html += "border:1px solid #ccc;";
    //html +=  "}\n";

    html += "</style>";

    // ----------------------------------------------------------

    auto it2 = cowboyScores->begin();

    if (cowboyScores->size() > 0)
    {
        // cowboy table
        html += "<table class='contentTable'>";
        html += "<caption><h1>" + username + "'s Cowboy Scores</h1><caption>";

        // build table headers
        html += "<tr>";
        html += "<th>Time</th>";
        html += "<th>Score</th>";
        html += "<th>WPM</th>";
        html += "<th>Words Typed</th>";
        html += "<th>Accuracy</th>";
        html += "<th>Difficulty</th>";
        html += "</tr>";



        for (auto it = cowboyStamps->begin(); it != cowboyStamps->end(); ++it)
        {

            std::stringstream ss;
            std::string current;

            html += "<tr>";

            html += "<td>" + *it + "</td>";

            ss.str("");
            ss << *it2;
            current = ss.str();
            html += "<td>" + current + "</td>"; it2++;

            ss.str("");
            ss << *it2;
            current = ss.str();
            html += "<td>" + current + "</td>"; it2++;

            ss.str("");
            ss << *it2;
            current = ss.str();
            html += "<td>" + current + "</td>"; it2++;

            ss.str("");
            ss << *it2;
            current = ss.str();
            html += "<td>" + current + "</td>"; it2++;

            ss.str("");
            ss << *it2;
            current = ss.str();
            html += "<td>" + current + "</td>"; it2++;

            html += "</tr>";
        }

        html += "</table><br>";

    }
    else
    {
        html += "No recorded scores for the cowboy game.<br><br>";
    }

    // ----------------------------------------------------------

    if (racingScores->size() > 0)
    {

        // racing table
        html += "<table class='contentTable'>";
        html += "<caption><h1>" + username + "'s Racing Scores</h1><caption>";

        // build table headers
        html += "<tr>";
        html += "<th>Time</th>";
        html += "<th>Score</th>";
        html += "<th>WPM</th>";
        html += "<th>Words Typed</th>";
        html += "<th>Accuracy</th>";
        html += "<th>Difficulty</th>";
        html += "</tr>";

        it2 = racingScores->begin();

        for (auto it = racingStamps->begin(); it != racingStamps->end(); ++it)
        {
            std::stringstream ss;
            std::string current;

            html += "<tr>";

            html += "<td>" + *it + "</td>";

            ss.str("");
            ss << *it2;
            current = ss.str();
            html += "<td>" + current + "</td>"; it2++;

            ss.str("");
            ss << *it2;
            current = ss.str();
            html += "<td>" + current + "</td>"; it2++;

            ss.str("");
            ss << *it2;
            current = ss.str();
            html += "<td>" + current + "</td>"; it2++;

            ss.str("");
            ss << *it2;
            current = ss.str();
            html += "<td>" + current + "</td>"; it2++;

            ss.str("");
            ss << *it2;
            current = ss.str();
            html += "<td>" + current + "</td>"; it2++;

            html += "</tr>";
        }

        html += "</table><br>";

    }
    else
    {
        html += "No recorded scores for the racing game.<br><br>";
    }

    // ----------------------------------------------------------

    if (fightingScores->size() > 0)
    {

        // fighting table
        html += "<table class='contentTable'>";
        html += "<caption><h1>" + username + "'s Fighting Scores</h1><caption>";

        // build table headers
        html += "<tr>";
        html += "<th>Time</th>";
        html += "<th>Score</th>";
        html += "<th>WPM</th>";
        html += "<th>Words Typed</th>";
        html += "<th>Accuracy</th>";
        html += "<th>Difficulty</th>";
        html += "</tr>";

        it2 = fightingScores->begin();

        for (auto it = fightingStamps->begin(); it != fightingStamps->end(); ++it)
        {
            std::stringstream ss;
            std::string current;

            html += "<tr>";

            html += "<td>" + *it + "</td>";

            ss.str("");
            ss << *it2;
            current = ss.str();
            html += "<td>" + current + "</td>"; it2++;

            ss.str("");
            ss << *it2;
            current = ss.str();
            html += "<td>" + current + "</td>"; it2++;

            ss.str("");
            ss << *it2;
            current = ss.str();
            html += "<td>" + current + "</td>"; it2++;

            ss.str("");
            ss << *it2;
            current = ss.str();
            html += "<td>" + current + "</td>"; it2++;

            ss.str("");
            ss << *it2;
            current = ss.str();
            html += "<td>" + current + "</td>"; it2++;

            html += "</tr>";
        }

        html += "</table><br>";

    }
    else
    {
        html += "<div>No recorded scores for the fighting game.</div><br><br>";
    }

    // ----------------------------------------------------------

    html += "<div>";

    // print player averages
    auto it = averageScores->begin();
    std::stringstream ss;

    ss.str("");
    std::string current;
    ss << *it;
    current = ss.str();
    html += "<strong>Average Score: <strong> " + current + "<br>"; it++;

    ss.str("");
    ss << *it;
    current = ss.str();
    html += "<strong>Average WPM: <strong> " + current + "<br>"; it++;

    ss.str("");
    ss << *it;
    current = ss.str();
    html += "<strong>Average Words Typed: <strong> " + current + "<br>"; it++;

    ss.str("");
    ss << *it;
    current = ss.str();
    html += "<strong>Average Accuracy: <strong>" + current + "<br>"; it++;

    ss.str("");
    ss << *it;
    current = ss.str();
    html += "<strong>Average Difficulty: <strong> " + current + "<br>"; it++;

    html += "</div>";

    // -----------------------------------------------------------------------------------

    html += "</html>";

    sf::Packet htmlPacket;
    htmlPacket << username << html;
    this->clientMap[clientName]->send(htmlPacket);





}

























