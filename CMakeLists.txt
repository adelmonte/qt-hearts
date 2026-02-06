cmake_minimum_required(VERSION 3.16)
project(qt-hearts VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Svg Multimedia Quick QuickWidgets QuickControls2)

set(SOURCES
    src/main.cpp
    src/card.cpp
    src/deck.cpp
    src/player.cpp
    src/game.cpp
    src/cardtheme.cpp
    src/cardimageprovider.cpp
    src/gamebridge.cpp
    src/soundengine.cpp
    resources.qrc
)

set(HEADERS
    include/card.h
    include/deck.h
    include/player.h
    include/game.h
    include/cardtheme.h
    include/cardimageprovider.h
    include/gamebridge.h
    include/soundengine.h
)

add_executable(qt-hearts ${SOURCES} ${HEADERS})

target_include_directories(qt-hearts PRIVATE include)
target_link_libraries(qt-hearts Qt6::Widgets Qt6::Svg Qt6::Multimedia Qt6::Quick Qt6::QuickWidgets Qt6::QuickControls2)

install(TARGETS qt-hearts DESTINATION bin)
install(FILES data/qt-hearts.desktop DESTINATION share/applications)
install(FILES data/icons/qt-hearts.svg DESTINATION share/icons/hicolor/scalable/apps)
install(DIRECTORY data/sounds/ DESTINATION share/qt-hearts/sounds)
