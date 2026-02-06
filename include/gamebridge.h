#ifndef GAMEBRIDGE_H
#define GAMEBRIDGE_H

#include "game.h"
#include "cardtheme.h"
#include "soundengine.h"
#include <QObject>
#include <QVariantList>
#include <QUrl>

class GameBridge : public QObject {
    Q_OBJECT

    // Core game state
    Q_PROPERTY(QVariantList playerHand READ playerHand NOTIFY playerHandChanged)
    Q_PROPERTY(QVariantList opponentCardCounts READ opponentCardCounts NOTIFY opponentCardCountsChanged)
    Q_PROPERTY(QVariantList trickCards READ trickCards NOTIFY trickCardsChanged)
    Q_PROPERTY(QVariantList players READ players NOTIFY playersChanged)
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)
    Q_PROPERTY(int passDirection READ passDirection NOTIFY passDirectionChanged)
    Q_PROPERTY(int gameState READ gameState NOTIFY gameStateChanged)
    Q_PROPERTY(bool inputBlocked READ inputBlocked NOTIFY inputBlockedChanged)
    Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectedCountChanged)
    Q_PROPERTY(bool gameOver READ gameOver NOTIFY gameOverChanged)
    Q_PROPERTY(int winner READ winner NOTIFY winnerChanged)
    Q_PROPERTY(bool undoAvailable READ undoAvailable NOTIFY undoAvailableChanged)

    // Settings
    Q_PROPERTY(QString themePath READ themePath WRITE setThemePath NOTIFY themePathChanged)
    Q_PROPERTY(int themeVersion READ themeVersion NOTIFY themeVersionChanged)
    Q_PROPERTY(qreal cardScale READ cardScale WRITE setCardScale NOTIFY cardScaleChanged)
    Q_PROPERTY(bool soundEnabled READ soundEnabled WRITE setSoundEnabled NOTIFY soundEnabledChanged)
    Q_PROPERTY(int aiDifficulty READ aiDifficulty WRITE setAIDifficulty NOTIFY aiDifficultyChanged)
    Q_PROPERTY(QVariantList availableThemes READ availableThemes CONSTANT)

    // Animation settings
    Q_PROPERTY(bool animateCardRotation READ animateCardRotation WRITE setAnimateCardRotation NOTIFY animateCardRotationChanged)
    Q_PROPERTY(bool animateAICards READ animateAICards WRITE setAnimateAICards NOTIFY animateAICardsChanged)
    Q_PROPERTY(bool animatePassingCards READ animatePassingCards WRITE setAnimatePassingCards NOTIFY animatePassingCardsChanged)

    // Game rules
    Q_PROPERTY(int endScore READ endScore WRITE setEndScore NOTIFY endScoreChanged)
    Q_PROPERTY(bool exactResetTo50 READ exactResetTo50 WRITE setExactResetTo50 NOTIFY exactResetTo50Changed)
    Q_PROPERTY(bool queenBreaksHearts READ queenBreaksHearts WRITE setQueenBreaksHearts NOTIFY queenBreaksHeartsChanged)
    Q_PROPERTY(bool moonProtection READ moonProtection WRITE setMoonProtection NOTIFY moonProtectionChanged)
    Q_PROPERTY(bool fullPolish READ fullPolish WRITE setFullPolish NOTIFY fullPolishChanged)

    // Theme preview
    Q_PROPERTY(int previewVersion READ previewVersion NOTIFY previewVersionChanged)

    // UI
    Q_PROPERTY(bool showMenuBar READ showMenuBar WRITE setShowMenuBar NOTIFY showMenuBarChanged)

    // Statistics
    Q_PROPERTY(int gamesPlayed READ gamesPlayed NOTIFY statisticsChanged)
    Q_PROPERTY(int gamesWon READ gamesWon NOTIFY statisticsChanged)
    Q_PROPERTY(double winRate READ winRate NOTIFY statisticsChanged)
    Q_PROPERTY(double avgScore READ avgScore NOTIFY statisticsChanged)
    Q_PROPERTY(int bestScore READ bestScore NOTIFY statisticsChanged)
    Q_PROPERTY(int shootTheMoonCount READ shootTheMoonCount NOTIFY statisticsChanged)

public:
    explicit GameBridge(QObject* parent = nullptr);
    ~GameBridge();

    // Core state
    QVariantList playerHand() const;
    QVariantList opponentCardCounts() const;
    QVariantList trickCards() const;
    QVariantList players() const;
    QString message() const { return m_message; }
    int passDirection() const;
    int gameState() const;
    bool inputBlocked() const { return m_inputBlocked; }
    int selectedCount() const { return m_selectedCards.size(); }
    bool gameOver() const { return m_gameOver; }
    int winner() const { return m_winner; }
    bool undoAvailable() const { return m_undoAvailable; }

    // Settings
    QString themePath() const;
    void setThemePath(const QString& path);
    int themeVersion() const { return m_themeVersion; }
    qreal cardScale() const { return m_cardScale; }
    void setCardScale(qreal scale);
    bool soundEnabled() const { return m_soundEnabled; }
    void setSoundEnabled(bool enabled);
    int aiDifficulty() const;
    void setAIDifficulty(int difficulty);
    QVariantList availableThemes() const;

    // Animation settings
    bool animateCardRotation() const { return m_animateCardRotation; }
    void setAnimateCardRotation(bool v);
    bool animateAICards() const { return m_animateAICards; }
    void setAnimateAICards(bool v);
    bool animatePassingCards() const { return m_animatePassingCards; }
    void setAnimatePassingCards(bool v);

    // Game rules
    int endScore() const;
    void setEndScore(int score);
    bool exactResetTo50() const;
    void setExactResetTo50(bool v);
    bool queenBreaksHearts() const;
    void setQueenBreaksHearts(bool v);
    bool moonProtection() const;
    void setMoonProtection(bool v);
    bool fullPolish() const;
    void setFullPolish(bool v);

    // Statistics
    int gamesPlayed() const { return m_gamesPlayed; }
    int gamesWon() const { return m_gamesWon; }
    double winRate() const { return m_gamesPlayed > 0 ? 100.0 * m_gamesWon / m_gamesPlayed : 0; }
    double avgScore() const { return m_gamesPlayed > 0 ? static_cast<double>(m_totalScore) / m_gamesPlayed : 0; }
    int bestScore() const { return m_bestScore == 999 ? -1 : m_bestScore; }
    int shootTheMoonCount() const { return m_shootTheMoonCount; }

    // Theme preview
    int previewVersion() const { return m_previewVersion; }
    CardTheme* previewTheme() const { return m_previewTheme; }

    // UI
    bool showMenuBar() const { return m_showMenuBar; }
    void setShowMenuBar(bool v);

    // Access to internal theme for image provider
    CardTheme* theme() const { return m_theme; }

    // Invokable methods
    Q_INVOKABLE void newGame();
    Q_INVOKABLE void undo();
    Q_INVOKABLE void quit();
    Q_INVOKABLE void cardClicked(int suit, int rank);
    Q_INVOKABLE QString cardImageSource(int suit, int rank) const;
    Q_INVOKABLE QString cardBackSource() const;
    Q_INVOKABLE QVariantList getValidPlays() const;
    Q_INVOKABLE void resetStatistics();
    Q_INVOKABLE QString scoresText() const;
    Q_INVOKABLE void loadPreviewTheme(const QString& path);

signals:
    // Core state
    void playerHandChanged();
    void opponentCardCountsChanged();
    void trickCardsChanged();
    void playersChanged();
    void messageChanged();
    void passDirectionChanged();
    void gameStateChanged();
    void inputBlockedChanged();
    void selectedCountChanged();
    void gameOverChanged();
    void winnerChanged();
    void undoAvailableChanged();

    // Settings
    void themePathChanged();
    void themeVersionChanged();
    void cardScaleChanged();
    void soundEnabledChanged();
    void aiDifficultyChanged();

    // Animation settings
    void animateCardRotationChanged();
    void animateAICardsChanged();
    void animatePassingCardsChanged();

    // Game rules
    void endScoreChanged();
    void exactResetTo50Changed();
    void queenBreaksHeartsChanged();
    void moonProtectionChanged();
    void fullPolishChanged();

    // Statistics
    void statisticsChanged();

    // Theme preview
    void previewVersionChanged();

    // UI
    void showMenuBarChanged();

    // Animation triggers
    void cardPlayedToTrick(int player, int suit, int rank, qreal fromX, qreal fromY);
    void trickWonByPlayer(int player, int points);
    void cardsReceived(QVariantList cards);
    void heartsBrokenSignal();

    // Dialog triggers (from native menu bar)
    void openScoresRequested();
    void openStatisticsRequested();
    void openSettingsRequested();
    void openAboutRequested();
    void toggleFullscreenRequested();

private slots:
    void onStateChanged(GameState state);
    void onCardsDealt();
    void onPassDirectionAnnounced(PassDirection dir);
    void onPassingComplete(Cards receivedCards);
    void onCardPlayed(int player, Card card);
    void onTrickWon(int winner, int points);
    void onRoundEnded();
    void onGameEnded(int winner);
    void onScoresChanged();
    void onCurrentPlayerChanged(int player);
    void onHeartsBroken();

private:
    void updateValidPlays();
    void showMessage(const QString& text, int durationMs = 2000);
    void hideMessage();
    void loadSettings();
    void saveSettings();

    Game* m_game;
    CardTheme* m_theme;
    CardTheme* m_previewTheme;
    SoundEngine* m_sound;
    QString m_message;
    bool m_inputBlocked = false;
    bool m_gameOver = false;
    int m_winner = -1;
    Cards m_selectedCards;
    Cards m_receivedCards;
    Cards m_validPlays;
    qreal m_cardScale = 1.0;
    int m_themeVersion = 0;
    int m_currentPlayer = -1;
    PassDirection m_passDirection = PassDirection::None;
    bool m_showingReceivedCards = false;
    bool m_passConfirmed = false;
    bool m_undoAvailable = false;
    bool m_soundEnabled = true;
    QTimer* m_messageTimer = nullptr;

    // Animation settings
    bool m_animateCardRotation = true;
    bool m_animateAICards = true;
    bool m_animatePassingCards = true;

    // Theme preview
    int m_previewVersion = 0;

    // UI
    bool m_showMenuBar = true;

    // Statistics
    int m_gamesPlayed = 0;
    int m_gamesWon = 0;
    int m_totalScore = 0;
    int m_bestScore = 999;
    int m_shootTheMoonCount = 0;
};

#endif // GAMEBRIDGE_H
