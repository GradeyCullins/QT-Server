#ifndef INITDB_H
#define INITDB_H

#include <QObject>
#include <mysql.h>
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <QDebug>
#include <sstream>
#include "server.h"

class Server;

class initdb : public QObject
{
    Q_OBJECT    
    MYSQL *dbConnection;
    Server *s;
    void connectSignals();
    std::list<double>* getTableScores(std::string username, std::string tableName);
    std::list<std::string>* getTimeStamps(std::string username, std::string tableName);

public:
    explicit initdb(QObject *parent = 0, Server* = 0);
    bool testConnection();

signals:
    void sendLoginResult(std::string, std::string, int, int);
    void sendRegisterResult(std::string, std::string);
    void sendPlayerStats(std::string, std::list<double>*);
    void sendPlayerNames(std::string, std::list<std::string>);
    void sendAllGameScores(std::string, std::string,std::list<double>*,std::list<double>*,std::list<double>*,std::list<double>*,std::list<std::string>*,std::list<std::string>*,std::list<std::string>*);

public slots:
    void onUserLogin(std::string, std::string);
    void onUserRegister(std::string, std::string);
    void onSendScores(std::string, std::string, std::list<double>*);
    void onUpdateLevel(std::string,double);
    void onQueryGameScores(std::string, std::string);
    void onGetPlayerNames(std::string);
    void onGetAllGameScores(std::string, std::string);
    void onDeleteUser(std::string);

};

#endif // INITDB_H
