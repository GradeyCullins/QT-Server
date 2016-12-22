QT += core
QT += sql
QT -= gui

QTPLUGIN += qsqlmysql

CONFIG += c++11

TARGET = Server
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    server.cpp \
    initdb.cpp \

LIBS += -L$$PWD/SFML/build-SFML-Desktop_Qt_5_5_1_MinGW_32bit-Default/lib

CONFIG(release, debug|release): LIBS += -lsfml-audio -lsfml-graphics -lsfml-main -lsfml-network -lsfml-window -lsfml-system

INCLUDEPATH += $$PWD/SFML/SFML/include
DEPENDPATH += $$PWD/SFML/SFML/include


# mysql libs mac
LIBS += -L/usr/local/mysql/lib/ -lmysqlclient -lm -lz

HEADERS += \
    server.h \
    initdb.h

# unix(linux) libraries/inclues
INCLUDEPATH += $$PWD/../../../usr/include/SFML
DEPENDPATH += $$PWD/../../../usr/include/SFML
INCLUDEPATH += $$PWD/../../../usr/lib/x86_64-linux-gnu
DEPENDPATH += $$PWD/../../../usr/lib/x86_64-linux-gnu
unix:!macx: LIBS += -L$$PWD/../../../usr/lib/x86_64-linux-gnu/ -lsfml-graphics
unix:!macx: LIBS += -L$$PWD/../../../usr/lib/x86_64-linux-gnu/ -lsfml-window
unix:!macx: LIBS += -L$$PWD/../../../usr/lib/x86_64-linux-gnu/ -lsfml-system
unix:!macx: LIBS += -L$$PWD/../../../usr/lib/x86_64-linux-gnu/ -lsfml-network

# mysql libraries/includes osx
macx: LIBS += -L/usr/local/mysql/lib/ -lmysqlclient -lm -lz
INCLUDEPATH += /usr/local/mysql/include
DEPENDPATH += /usr/local/mysql/include

# osx includes and library linking
macx: LIBS += -L$$PWD/../lib/ -lsfml-network
macx: LIBS += -L$$PWD/../lib/ -lsfml-system
macx: LIBS += -L$$PWD/../lib/ -lsfml-window
macx: LIBS += -L$$PWD/../lib/ -lsfml-graphics.2.3.2
INCLUDEPATH += $$PWD/../../../../../usr/local/include
DEPENDPATH += $$PWD/../../../../../usr/local/include

# linux mysql include/linking
unix:!macx: LIBS += -L/usr/lib/x86_64-linux-gnu/ -lmysqlclient -lm -lz
INCLUDEPATH += /usr/include/mysql/
DEPENDPATH += /usr/include/mysql/

# linux sfml include/linking
# note: I have started the habit of keeping the libraries in my local repository
unix:!macx: LIBS += -L$$PWD/../libs/SFML-master/lib/ -lsfml-audio
unix:!macx: LIBS += -L$$PWD/../libs/SFML-master/lib/ -lsfml-graphics
unix:!macx: LIBS += -L$$PWD/../libs/SFML-master/lib/ -lsfml-network
unix:!macx: LIBS += -L$$PWD/../libs/SFML-master/lib/ -lsfml-system
unix:!macx: LIBS += -L$$PWD/../libs/SFML-master/lib/ -lsfml-window
INCLUDEPATH += $$PWD/../libs/SFML-master/include
DEPENDPATH += $$PWD/../libs/SFML-master/include
