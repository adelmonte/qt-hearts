QT += core gui widgets svgwidgets multimedia quick quickcontrols2 quickwidgets

CONFIG += c++17

TARGET = qt-hearts
TEMPLATE = app

INCLUDEPATH += include

SOURCES += \
    src/main.cpp \
    src/card.cpp \
    src/deck.cpp \
    src/player.cpp \
    src/game.cpp \
    src/cardtheme.cpp \
    src/gamebridge.cpp \
    src/cardimageprovider.cpp \
    src/soundengine.cpp

HEADERS += \
    include/card.h \
    include/deck.h \
    include/player.h \
    include/game.h \
    include/cardtheme.h \
    include/gamebridge.h \
    include/cardimageprovider.h \
    include/soundengine.h

RESOURCES += resources.qrc qml.qrc
