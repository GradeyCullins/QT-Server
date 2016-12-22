#include <QCoreApplication>
#include <SFML/Network.hpp>
#include <iostream>
#include <QPluginLoader>
#include "server.h"
#include "initdb.h"

unsigned int PORT = 40003;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);        

    Server server(PORT);

    return a.exec();
}
