QT += core gui widgets svgwidgets multimedia openglwidgets

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
    src/carditem.cpp \
    src/gameview.cpp \
    src/mainwindow.cpp \
    src/soundengine.cpp \
    src/smoothanimationdriver.cpp

HEADERS += \
    include/card.h \
    include/deck.h \
    include/player.h \
    include/game.h \
    include/cardtheme.h \
    include/carditem.h \
    include/gameview.h \
    include/mainwindow.h \
    include/soundengine.h \
    include/smoothanimationdriver.h

RESOURCES += resources.qrc
