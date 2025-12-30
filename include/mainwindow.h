#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include "player.h"  // For AIDifficulty enum
#include "game.h"    // For GameRules struct

class GameView;
class CardTheme;
class SoundEngine;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void newGame();
    void undo();
    void showSettings();
    void showAbout();
    void showScores();
    void showStatistics();
    void toggleFullscreen();
    void toggleMenuBar();
    void onGameEnded(int winner);
    void onUndoAvailableChanged(bool available);

private:
    void createActions();
    void createMenus();
    void loadSettings();
    void saveSettings();
    void loadTheme(const QString& themePath);

    GameView* m_gameView;
    std::unique_ptr<Game> m_game;
    std::unique_ptr<CardTheme> m_theme;
    std::unique_ptr<SoundEngine> m_sound;

    QAction* m_newGameAction;
    QAction* m_undoAction;
    QAction* m_settingsAction;
    QAction* m_quitAction;
    QAction* m_aboutAction;
    QAction* m_scoresAction;
    QAction* m_statisticsAction;
    QAction* m_fullscreenAction;
    QAction* m_hideMenuAction;

    QString m_currentThemePath;
    qreal m_cardScale;
    bool m_soundEnabled;
    AIDifficulty m_aiDifficulty;
    GameRules m_gameRules;

    // Statistics
    int m_gamesPlayed;
    int m_gamesWon;
    int m_totalScore;
    int m_bestScore;
    int m_shootTheMoonCount;
};

#endif // MAINWINDOW_H
