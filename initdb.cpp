#include "initdb.h"

/*
 * CONSTRUCTOR:
 * Set up a DB connection, store it for later use.
 */
initdb::initdb(QObject *parent, Server *_server) : QObject(parent), s(_server)
{
    connectSignals();
}

bool initdb::testConnection()
{
    MYSQL mysql;

    mysql_init(&mysql);

    dbConnection = mysql_real_connect(&mysql,"<redacted>","cs3505_dropTable","<redacted>","cs3505_dropTable",3306,0,0);

    if (dbConnection == NULL) // if the connection errored, print the sql error
    {
        std::cout << mysql_error(&mysql) << std::endl;
        return false;
    }
    else {  // succesful connection
        std::cout << "Connection to database was successful" << std::endl;
        mysql_close(dbConnection);
        return true;
    }
}

void initdb::connectSignals()
{
    QObject::connect(this, SIGNAL(sendLoginResult(std::string, std::string, int, int)), this->s, SLOT(onSendLoginResult(std::string, std::string, int, int)));
    QObject::connect(this, SIGNAL(sendRegisterResult(std::string,std::string)), this->s, SLOT(onSendRegisterResult(std::string,std::string)));
    QObject::connect(this, SIGNAL(sendPlayerStats(std::string,std::list<double>*)), this->s, SLOT(onSendPlayerStats(std::string,std::list<double>*)));
    QObject::connect(this, SIGNAL(sendPlayerNames(std::string,std::list<std::string>)), this->s, SLOT(onSendPlayerNames(std::string,std::list<std::string>)));
    QObject::connect(this, SIGNAL(sendAllGameScores(std::string, std::string,std::list<double>*,std::list<double>*,std::list<double>*,std::list<double>*,std::list<std::string>*,std::list<std::string>*,std::list<std::string>*)), this->s, SLOT(onSendAllGameScores(std::string, std::string,std::list<double>*,std::list<double>*,std::list<double>*,std::list<double>*,std::list<std::string>*,std::list<std::string>*,std::list<std::string>*)));
}

//-----------------------------------------------------------------------------------
// SLOTS
//-----------------------------------------------------------------------------------

/*
 * This slot fires when a new user attempts to register a new profile when interacting with the login page.
 */
void initdb::onUserRegister(std::string username, std::string password)
{
    MYSQL mysql;
    MYSQL_RES *result;
    int state;

    mysql_init(&mysql);

    dbConnection = mysql_real_connect(&mysql,"scottcullins.com","cs3505_dropTable","password123","cs3505_dropTable",3306,0,0);

    // if the connection errored, print the sql error
    if (dbConnection != NULL)
    {
        // build sql variable routine strings
        std::string u = "SET @username='" + username + "'";
        std::string p = "SET @password='" + password + "'";

        // assign userName and password arguments to sql variables
        mysql_query(dbConnection, u.c_str());
        mysql_query(dbConnection, p.c_str());

        // check if the user already exists in the db
        state = mysql_query(dbConnection, "select * from cs3505_dropTable.Users where USERNAME=@username");

        if (state !=0) std::cout << mysql_error(this->dbConnection) << std::endl;

        result = mysql_store_result(this->dbConnection);

        // if the user already exists. . .
        if (mysql_num_rows(result) == 1)
        {
            emit sendRegisterResult(username, "USER_ALREADY_EXISTS");
        }
        else
        {
            state = mysql_query(dbConnection, "insert into cs3505_dropTable.Users (USERNAME, PASSWORD) values (@username, @password)");

            if (state !=0)
            {
                std::cout << mysql_error(this->dbConnection) << std::endl;
            }
            else
            {
                std::cout << "added new user " << username << " into database!"  << std::endl;
                emit sendRegisterResult(username, "USER_CREATED");
            }
        }
    }

    mysql_close(dbConnection);
}

/*
 * This slot fires when the login page sends a signal indicating that a user is attempting to login/authenticate. Sends a signal
 * to the login page confirming or disconfirming an attempted login.
 */
void initdb::onUserLogin(std::string username, std::string password)
{      
    int state;
    MYSQL_RES *result;
    MYSQL_ROW currentRow;
    MYSQL mysql;

    mysql_init(&mysql);

    dbConnection = mysql_real_connect(&mysql,"scottcullins.com","cs3505_dropTable","password123","cs3505_dropTable",3306,0,0);

    // if the connection errored, print the sql error
    if (dbConnection != NULL)
    {
        // build sql variable routine strings
        std::string u = "SET @username='" + username + "'";
        std::string p = "SET @password='" + password + "'";

        // assign userName and password arguments to sql variables
        mysql_query(dbConnection, u.c_str());
        mysql_query(dbConnection, p.c_str());

        state = mysql_query(this->dbConnection, "select * from cs3505_dropTable.Users where USERNAME=@username");

        if (state !=0)
        {
            std::cout << mysql_error(this->dbConnection) << std::endl;
        }

        result = mysql_store_result(this->dbConnection);

        if (mysql_num_rows(result) == 1)
        {
            currentRow = mysql_fetch_row(result);

            // if password matches "password", login was successful
            if (currentRow[1] == password)
            {
                std::stringstream ss;
                ss << currentRow[2];
                unsigned int isAdmin;
                ss >> isAdmin;

                std::stringstream ss2;
                ss2 << currentRow[3];
                unsigned int highestLevel;
                ss2 >> highestLevel;
                emit sendLoginResult(username, "SUCCESS", isAdmin, highestLevel);
            }
            else // if password was incorrect
            {
                emit sendLoginResult(username, "WRONG_PASSWORD", 0, 1);
            }
        }
        else // if username does not exist
        {
            emit sendLoginResult(username, "USER_NOT_FOUND", 0, 1);
        }
    }

    mysql_close(dbConnection);
}

/*
 * Triggered when the server signals to insert a new player score entry. Inserts the scores into the respective table for player's latest session.
 */
void initdb::onSendScores(std::string username, std::string table, std::list<double>* scores)
{
    int state;
    MYSQL mysql;

    mysql_init(&mysql);

    dbConnection = mysql_real_connect(&mysql,"scottcullins.com","cs3505_dropTable","password123","cs3505_dropTable",3306,0,0);

    // if the connection errored, print the sql error
    if (dbConnection != NULL)
    {
        // build sql variable routine strings
        std::string u = "SET @username='" + username + "'";

        // assign userName and password arguments to sql variables
        mysql_query(dbConnection, u.c_str());

        /*
         * THIS CODE = ME ON SUICIDE WATCH
         */
        std::stringstream s;
        s << "SET @score='" << *scores->begin() << "'";
        mysql_query(dbConnection, s.str().c_str());

        scores->pop_front();
        s.str("");
        s << "SET @wpm='" << *scores->begin() << "'";
        mysql_query(dbConnection, s.str().c_str());

        scores->pop_front();
        s.str("");
        s << "SET @words_typed='" << *scores->begin() << "'";
        mysql_query(dbConnection, s.str().c_str());

        scores->pop_front();
        s.str("");
        s << "SET @accuracy='" << *scores->begin() << "'";
        mysql_query(dbConnection, s.str().c_str());

        scores->pop_front();
        s.str("");
        s << "SET @difficulty='" << *scores->begin() << "'";
        mysql_query(dbConnection, s.str().c_str());

        if (table == "COWBOY_SCORES")
        {
            state = mysql_query(this->dbConnection, "INSERT INTO `cs3505_dropTable`.`COWBOY_SCORES` (`DATE_RECORDED`, `USERNAME`, `SCORE`, `WPM`, `WORDS_TYPED`, `ACCURACY`, `DIFFICULTY`) VALUES (NOW(), @username, @score, @wpm, @words_typed, @accuracy, @difficulty);");
        }
        else if (table == "RACING_SCORES")
        {
            state = mysql_query(this->dbConnection, "INSERT INTO `cs3505_dropTable`.`RACING_SCORES` (`DATE_RECORDED`, `USERNAME`, `SCORE`, `WPM`, `WORDS_TYPED`, `ACCURACY`, `DIFFICULTY`) VALUES (NOW(), @username, @score, @wpm, @words_typed, @accuracy, @difficulty);");

        }
        else if (table == "FIGHTING_SCORES")
        {
            state = mysql_query(this->dbConnection, "INSERT INTO `cs3505_dropTable`.`FIGHTING_SCORES` (`DATE_RECORDED`, `USERNAME`, `SCORE`, `WPM`, `WORDS_TYPED`, `ACCURACY`, `DIFFICULTY`) VALUES (NOW(), @username, @score, @wpm, @words_typed, @accuracy, @difficulty);");
        }

        if (state !=0) // the insertion went afoul
        {
            std::cout << mysql_error(this->dbConnection) << std::endl;
        }
        else
        {
            std::cout << "inserted scores for " << username << " into db" << std::endl;
        }
    }

    mysql_close(dbConnection);
    onQueryGameScores(table, username);
}

/*
 * Update username's row to properly reflect the currently attained level
 */
void initdb::onUpdateLevel(std::string username, double level)
{
    int state;
    MYSQL_RES *result;
    MYSQL mysql;

    mysql_init(&mysql);

    dbConnection = mysql_real_connect(&mysql,"scottcullins.com","cs3505_dropTable","password123","cs3505_dropTable",3306,0,0);

    // if the connection errored, print the sql error
    if (dbConnection != NULL)
    {
        // build sql variable routine strings
        std::string u = "SET @username='" + username + "'";

        std::stringstream s;
        s << "SET @level='" << level << "'";

        // assign userName and level arguments to sql variables
        mysql_query(dbConnection, u.c_str());
        mysql_query(dbConnection, s.str().c_str());

        state = mysql_query(this->dbConnection, "UPDATE cs3505_dropTable.Users SET CURRENT_LEVEL=@level WHERE USERNAME=@username;");

        if (state !=0)
        {
            std::cout << mysql_error(this->dbConnection) << std::endl;
        }
        else
        {
            std::cout << "updated " << username << "'s level" << std::endl;
        }

        result = mysql_store_result(this->dbConnection);
    }

    mysql_close(dbConnection);
}

/*
 * Retrieve a collection of a player statistics for gametype 'game'
 */
void initdb::onQueryGameScores(std::string game, std::string username)
{
    int state;
    MYSQL_RES *result;
    MYSQL_ROW currentRow;
    MYSQL mysql;

    mysql_init(&mysql);

    this->dbConnection = mysql_real_connect(&mysql,"scottcullins.com","cs3505_dropTable","password123","cs3505_dropTable",3306,0,0);

    // if the connection errored, print the sql error
    if (this->dbConnection != NULL)
    {
        // build sql variable routine strings
        std::string u = "SET @username='" + username + "'";

        // assign userName and password arguments to sql variables
        mysql_query(this->dbConnection, u.c_str());

        if (game == "COWBOY_SCORES")
        {
            state = mysql_query(this->dbConnection, "select * from cs3505_dropTable.COWBOY_SCORES where USERNAME=@username");
        }
        else if (game == "RACING_SCORES")
        {
            state = mysql_query(this->dbConnection, "select * from cs3505_dropTable.RACING_SCORES where USERNAME=@username");
        }
        else if (game == "FIGHTING_SCORES")
        {
            state = mysql_query(this->dbConnection, "select * from cs3505_dropTable.FIGHTING_SCORES where USERNAME=@username");
        }

        if (state !=0)
        {
            std::cout << mysql_error(this->dbConnection) << std::endl;
        }

        result = mysql_store_result(this->dbConnection);

        if (mysql_num_rows(result) > 0)
        {
            double score, wpm, wordsTyped, accuracy, difficulty;
            double averageScore, averageWpm, averageWordsTyped, averageAccuracy, averageDifficulty, averagePosition, averageTime;
            averageScore = averageWpm = averageWordsTyped = averageAccuracy = averageDifficulty = averagePosition = averageTime = 0;

            while (currentRow = mysql_fetch_row(result)) // sum all scores for a given player on a given game
            {
                score = atof(currentRow[2]);
                wpm = atof(currentRow[3]);
                wordsTyped = atof(currentRow[4]);
                accuracy = atof(currentRow[5]);
                difficulty = atof(currentRow[6]);

                averageScore += score;
                averageWpm += wpm;
                averageWordsTyped += wordsTyped;
                averageAccuracy += accuracy;
                averageDifficulty += difficulty;
            }

            // calculate averages
            averageScore /= mysql_num_rows(result);
            averageWpm /= mysql_num_rows(result);
            averageWordsTyped /= mysql_num_rows(result);
            averageAccuracy /= mysql_num_rows(result);
            averageDifficulty /= mysql_num_rows(result);

            // send averages to client
            emit sendPlayerStats(username, new std::list<double> {averageScore, averageWpm, averageWordsTyped, averageAccuracy, averageDifficulty});
        }
        else // if no recorded scores were found
        {
            std::cout << "No scores were found for " << username << std::endl;
        }
    }

    mysql_close(dbConnection);
}

void initdb::onGetPlayerNames(std::string username)
{
    int state;
    MYSQL_RES *result;
    MYSQL_ROW currentRow;
    MYSQL mysql;

    mysql_init(&mysql);

    this->dbConnection = mysql_real_connect(&mysql,"scottcullins.com","cs3505_dropTable","password123","cs3505_dropTable",3306,0,0);

    // if the connection errored, print the sql error
    if (this->dbConnection != NULL)
    {
        // build sql variable routine strings
        std::string u = "SET @username='" + username + "'";

        // assign userName and password arguments to sql variables
        mysql_query(this->dbConnection, u.c_str());

        state = mysql_query(this->dbConnection, "select * from cs3505_dropTable.Users");

        if (state !=0)
        {
            std::cout << mysql_error(this->dbConnection) << std::endl;
        }

        result = mysql_store_result(this->dbConnection);

        std::list<std::string> names;
        if (mysql_num_rows(result) > 0)
        {
            while (currentRow = mysql_fetch_row(result)) // sum all scores for a given player on a given game
            {
                names.push_back(currentRow[0]);
            }

            // send averages to client
            emit sendPlayerNames(username, names);
        }
        else // if no recorded scores were found
        {
            std::cout << "No players found: " << std::endl;
        }
    }

    mysql_close(dbConnection);

}

void initdb::onDeleteUser(std::string username)
{
    int state;
    MYSQL mysql;
    mysql_init(&mysql);

    this->dbConnection = mysql_real_connect(&mysql,"scottcullins.com","cs3505_dropTable","password123","cs3505_dropTable",3306,0,0);

    // if the connection errored, print the sql error
    if (this->dbConnection != NULL)
    {
        // build sql variable routine strings
        std::string u = "SET @username='" + username + "'";

        // assign userName and to sql variable
        mysql_query(this->dbConnection, u.c_str());

        state = mysql_query(this->dbConnection, "DELETE FROM cs3505_dropTable.RACING_SCORES WHERE USERNAME=@username;");
        state = mysql_query(this->dbConnection, "DELETE FROM cs3505_dropTable.FIGHTING_SCORES WHERE USERNAME=@username;");
        state = mysql_query(this->dbConnection, "DELETE FROM cs3505_dropTable.COWBOY_SCORES WHERE USERNAME=@username;");
        state = mysql_query(this->dbConnection, "DELETE FROM cs3505_dropTable.Users WHERE USERNAME=@username;");

        if (state !=0)
        {
            std::cout << mysql_error(this->dbConnection) << std::endl;
        }
        else
        {
            std::cout << "Deleted " << username << " from DB!" << std::endl;
        }
    }
    mysql_close(dbConnection);
}

std::list<double>* initdb::getTableScores(std::string username, std::string tableName)
{
    int state;
    MYSQL_RES *result;
    MYSQL_ROW currentRow;
    MYSQL mysql;
    mysql_init(&mysql);
    std::list<double>* scores = new std::list<double>;

    this->dbConnection = mysql_real_connect(&mysql,"scottcullins.com","cs3505_dropTable","password123","cs3505_dropTable",3306,0,0);

    // if the connection errored, print the sql error
    if (this->dbConnection != NULL)
    {
        // build sql variable routine strings
        std::string u = "SET @username='" + username + "'";

        // assign userName and to sql variable
        mysql_query(this->dbConnection, u.c_str());

        std::string queryString;
        queryString = std::string("SELECT * FROM cs3505_dropTable.") + tableName + std::string(" WHERE USERNAME=@username;");

        state = mysql_query(this->dbConnection, queryString.c_str());

        result = mysql_store_result(this->dbConnection);

        double score, wpm, wordsTyped, accuracy, difficulty;

        if (state !=0)
        {
            std::cout << mysql_error(this->dbConnection) << std::endl;
        }
        else
        {
            while(currentRow = mysql_fetch_row(result)){
                score = atof(currentRow[2]);
                wpm = atof(currentRow[3]);
                wordsTyped = atof(currentRow[4]);
                accuracy = atof(currentRow[5]);
                difficulty = atof(currentRow[6]);

                scores->push_back(score);
                scores->push_back(wpm);
                scores->push_back(wordsTyped);
                scores->push_back(accuracy);
                scores->push_back(difficulty);
            }
        }
    }
    mysql_close(dbConnection);
    return scores;
}

std::list<std::string>* initdb::getTimeStamps(std::string username, std::string tableName)
{
    int state;
    MYSQL_RES *result;
    MYSQL_ROW currentRow;
    MYSQL mysql;
    mysql_init(&mysql);
    std::list<std::string>* stamps = new std::list<std::string>;

    this->dbConnection = mysql_real_connect(&mysql,"scottcullins.com","cs3505_dropTable","password123","cs3505_dropTable",3306,0,0);

    // if the connection errored, print the sql error
    if (this->dbConnection != NULL)
    {
        // build sql variable routine strings
        std::string u = "SET @username='" + username + "'";

        // assign userName and to sql variable
        mysql_query(this->dbConnection, u.c_str());

        std::string queryString;
        queryString = std::string("SELECT * FROM cs3505_dropTable.") + tableName + std::string(" WHERE USERNAME=@username;");

        state = mysql_query(this->dbConnection, queryString.c_str());

        result = mysql_store_result(this->dbConnection);

        std::string stamp;

        if (state !=0)
        {
            std::cout << mysql_error(this->dbConnection) << std::endl;
        }
        else
        {
            while(currentRow = mysql_fetch_row(result)){
                stamp = currentRow[0];
                stamps->push_back(stamp);
            }
        }
    }
    mysql_close(dbConnection);
    return stamps;
}

void initdb::onGetAllGameScores(std::string username, std::string clientName)
{
    std::list<double>* cowboyScores = getTableScores(username, "COWBOY_SCORES");
    std::list<double>* racingScores = getTableScores(username, "RACING_SCORES");
    std::list<double>* fightingScores = getTableScores(username, "FIGHTING_SCORES");
    std::list<double>* allScores = new std::list<double>;

    std::list<std::string>* cowboyStamps = getTimeStamps(username, "COWBOY_SCORES");
    std::list<std::string>* racingStamps = getTimeStamps(username, "RACING_SCORES");
    std::list<std::string>* fightingStamps = getTimeStamps(username, "FIGHTING_SCORES");

    double averageScore, averageWpm, averageWordsTyped, averageAccuracy, averageDifficulty, averagePosition, averageTime;
    averageScore = averageWpm = averageWordsTyped = averageAccuracy = averageDifficulty = averagePosition = averageTime = 0;

    for(auto it = cowboyScores->begin(); it != cowboyScores->end(); it++){
        allScores->push_back(*it);
    }
    for(auto it = racingScores->begin(); it != racingScores->end(); it++){
        allScores->push_back(*it);
    }
    for(auto it = fightingScores->begin(); it != fightingScores->end(); it++){
        allScores->push_back(*it);
    }

    int i = 0;
    for (auto it = allScores->begin(); it != allScores->end(); ++it)
    {
        if (i % 5 == 0)
        {
            averageScore += *it;
        }
        else if (i % 5 == 1)
        {
            averageWpm += *it;
        }
        else if (i % 5 == 2)
        {
            averageWordsTyped += *it;
        }
        else if (i % 5 == 3)
        {
            averageAccuracy += *it;
        }
        else if (i % 5 == 4)
        {
            averageDifficulty += *it;
        }
        i++;
    }

    averageScore /= (allScores->size() / 5);
    averageWpm /= (allScores->size() / 5);
    averageWordsTyped /= (allScores->size() / 5);
    averageAccuracy /= (allScores->size() / 5);
    averageDifficulty /= (allScores->size() / 5);

    std::list<double>* averageScores = new std::list<double> {averageScore, averageWpm, averageWordsTyped, averageAccuracy, averageDifficulty};

    emit sendAllGameScores(username, clientName, cowboyScores, racingScores, fightingScores, averageScores, cowboyStamps, racingStamps, fightingStamps);
}

