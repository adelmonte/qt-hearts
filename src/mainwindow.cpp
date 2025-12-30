#include "mainwindow.h"
#include "gameview.h"
#include "game.h"
#include "cardtheme.h"
#include "soundengine.h"
#include "player.h"
#include "card.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include <QFrame>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QApplication>
#include <QSlider>
#include <QTimer>
#include <QShortcut>
#include <QCheckBox>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_gameView(new GameView(this))
    , m_game(std::make_unique<Game>())
    , m_theme(std::make_unique<CardTheme>())
    , m_sound(std::make_unique<SoundEngine>(this))
    , m_soundEnabled(true)
    , m_gamesPlayed(0)
    , m_gamesWon(0)
    , m_totalScore(0)
    , m_bestScore(999)
    , m_shootTheMoonCount(0)
{
    setWindowTitle("Hearts");
    setMinimumSize(800, 600);
    resize(1024, 768);

    setCentralWidget(m_gameView);

    createActions();
    createMenus();
    loadSettings();

    m_gameView->setGame(m_game.get());
    m_gameView->setTheme(m_theme.get());

    // Connect sound effects to game events
    connect(m_game.get(), &Game::cardsDealt, m_sound.get(), &SoundEngine::playCardShuffle);
    connect(m_game.get(), &Game::cardPlayed, m_sound.get(), &SoundEngine::playCardPutDown);
    connect(m_game.get(), &Game::gameEnded, this, [this](int winner) {
        if (winner == 0) {
            m_sound->playWin();
        } else {
            m_sound->playLose();
        }
    });

    // Track game endings for statistics
    connect(m_game.get(), &Game::gameEnded, this, &MainWindow::onGameEnded);

    // Undo functionality
    connect(m_game.get(), &Game::undoAvailableChanged, this, &MainWindow::onUndoAvailableChanged);
    connect(m_game.get(), &Game::undoPerformed, m_gameView, &GameView::onUndoPerformed);

    // Auto-start a new game
    QTimer::singleShot(100, this, &MainWindow::newGame);
}

MainWindow::~MainWindow() {
    saveSettings();
}

void MainWindow::createActions() {
    // Game menu actions
    m_newGameAction = new QAction(QIcon::fromTheme("document-new"), tr("&New Game"), this);
    m_newGameAction->setShortcut(QKeySequence::New);
    connect(m_newGameAction, &QAction::triggered, this, &MainWindow::newGame);

    m_undoAction = new QAction(QIcon::fromTheme("edit-undo"), tr("&Undo"), this);
    m_undoAction->setShortcut(QKeySequence::Undo);
    m_undoAction->setEnabled(false);
    connect(m_undoAction, &QAction::triggered, this, &MainWindow::undo);

    m_scoresAction = new QAction(QIcon::fromTheme("view-statistics"), tr("&Scores"), this);
    connect(m_scoresAction, &QAction::triggered, this, &MainWindow::showScores);

    m_statisticsAction = new QAction(QIcon::fromTheme("games-highscores"), tr("&High Scores..."), this);
    connect(m_statisticsAction, &QAction::triggered, this, &MainWindow::showStatistics);

    m_quitAction = new QAction(QIcon::fromTheme("application-exit"), tr("&Quit"), this);
    m_quitAction->setShortcut(QKeySequence::Quit);
    connect(m_quitAction, &QAction::triggered, this, &QWidget::close);

    // View menu actions
    m_fullscreenAction = new QAction(QIcon::fromTheme("view-fullscreen"), tr("F&ull Screen Mode"), this);
    m_fullscreenAction->setShortcut(QKeySequence::FullScreen);
    m_fullscreenAction->setCheckable(true);
    connect(m_fullscreenAction, &QAction::triggered, this, &MainWindow::toggleFullscreen);

    m_hideMenuAction = new QAction(tr("&Show Menu Bar"), this);
    m_hideMenuAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_M));
    m_hideMenuAction->setCheckable(true);
    m_hideMenuAction->setChecked(true);
    connect(m_hideMenuAction, &QAction::triggered, this, &MainWindow::toggleMenuBar);

    // Settings menu actions
    m_settingsAction = new QAction(QIcon::fromTheme("configure"), tr("&Configure Hearts..."), this);
    m_settingsAction->setShortcut(QKeySequence::Preferences);
    connect(m_settingsAction, &QAction::triggered, this, &MainWindow::showSettings);

    // Help menu actions
    m_aboutAction = new QAction(QIcon::fromTheme("help-about"), tr("&About Hearts"), this);
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
}

void MainWindow::createMenus() {
    // Game menu
    QMenu* gameMenu = menuBar()->addMenu(tr("&Game"));
    gameMenu->addAction(m_newGameAction);
    gameMenu->addAction(m_undoAction);
    gameMenu->addSeparator();
    gameMenu->addAction(m_scoresAction);
    gameMenu->addAction(m_statisticsAction);
    gameMenu->addSeparator();
    gameMenu->addAction(m_quitAction);

    // View menu
    QMenu* viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(m_fullscreenAction);
    viewMenu->addAction(m_hideMenuAction);

    // Settings menu (KDE convention - separate from Game)
    QMenu* settingsMenu = menuBar()->addMenu(tr("&Settings"));
    settingsMenu->addAction(m_settingsAction);

    // Help menu
    QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(QIcon::fromTheme("help-contents"), tr("&Hearts Handbook"), this, []() {
        // Could open documentation if we had any
    })->setEnabled(false);
    helpMenu->addSeparator();
    helpMenu->addAction(m_aboutAction);
    helpMenu->addAction(QIcon::fromTheme("help-about-qt"), tr("About &Qt"), qApp, &QApplication::aboutQt);
}

void MainWindow::loadSettings() {
    QSettings settings("Hearts", "Hearts");

    m_currentThemePath = settings.value("theme", "").toString();

    if (m_currentThemePath.isEmpty() || !m_theme->loadTheme(m_currentThemePath)) {
        // Try to find a theme
        QVector<ThemeInfo> themes = CardTheme::findThemes();
        if (!themes.isEmpty()) {
            m_currentThemePath = themes.first().path;
            m_theme->loadTheme(m_currentThemePath);
        } else {
            m_theme->loadBuiltinTheme();
        }
    }

    m_cardScale = settings.value("cardScale", 1.0).toDouble();
    m_gameView->setCardScale(m_cardScale);

    m_soundEnabled = settings.value("soundEnabled", true).toBool();
    m_sound->setEnabled(m_soundEnabled);

    // AI Difficulty
    m_aiDifficulty = static_cast<AIDifficulty>(settings.value("aiDifficulty", static_cast<int>(AIDifficulty::Medium)).toInt());
    m_game->setAIDifficulty(m_aiDifficulty);

    // Game rules
    m_gameRules.endScore = settings.value("rules/endScore", 100).toInt();
    m_gameRules.exactResetTo50 = settings.value("rules/exactResetTo50", false).toBool();
    m_gameRules.queenBreaksHearts = settings.value("rules/queenBreaksHearts", true).toBool();
    m_gameRules.moonProtection = settings.value("rules/moonProtection", false).toBool();
    m_gameRules.fullPolish = settings.value("rules/fullPolish", false).toBool();
    m_game->setRules(m_gameRules);

    // Statistics
    m_gamesPlayed = settings.value("stats/gamesPlayed", 0).toInt();
    m_gamesWon = settings.value("stats/gamesWon", 0).toInt();
    m_totalScore = settings.value("stats/totalScore", 0).toInt();
    m_bestScore = settings.value("stats/bestScore", 999).toInt();
    m_shootTheMoonCount = settings.value("stats/shootTheMoon", 0).toInt();

    QByteArray geometry = settings.value("geometry").toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
}

void MainWindow::saveSettings() {
    QSettings settings("Hearts", "Hearts");
    settings.setValue("theme", m_currentThemePath);
    settings.setValue("cardScale", m_cardScale);
    settings.setValue("soundEnabled", m_soundEnabled);
    settings.setValue("aiDifficulty", static_cast<int>(m_aiDifficulty));
    settings.setValue("geometry", saveGeometry());

    // Game rules
    settings.setValue("rules/endScore", m_gameRules.endScore);
    settings.setValue("rules/exactResetTo50", m_gameRules.exactResetTo50);
    settings.setValue("rules/queenBreaksHearts", m_gameRules.queenBreaksHearts);
    settings.setValue("rules/moonProtection", m_gameRules.moonProtection);
    settings.setValue("rules/fullPolish", m_gameRules.fullPolish);

    // Statistics
    settings.setValue("stats/gamesPlayed", m_gamesPlayed);
    settings.setValue("stats/gamesWon", m_gamesWon);
    settings.setValue("stats/totalScore", m_totalScore);
    settings.setValue("stats/bestScore", m_bestScore);
    settings.setValue("stats/shootTheMoon", m_shootTheMoonCount);
}

void MainWindow::loadTheme(const QString& themePath) {
    if (m_theme->loadTheme(themePath)) {
        m_currentThemePath = themePath;
        m_gameView->setTheme(m_theme.get());
    }
}

void MainWindow::newGame() {
    m_game->newGame();
}

void MainWindow::showSettings() {
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Settings"));
    dialog.setMinimumWidth(500);
    dialog.setMinimumHeight(400);

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // Theme selection
    QHBoxLayout* themeLayout = new QHBoxLayout;
    themeLayout->addWidget(new QLabel(tr("Card Theme:")));

    QComboBox* themeCombo = new QComboBox;
    themeCombo->addItem(tr("Built-in"), "");

    QVector<ThemeInfo> themes = CardTheme::findThemes();
    int currentIndex = 0;
    for (int i = 0; i < themes.size(); ++i) {
        themeCombo->addItem(themes[i].name, themes[i].path);
        if (themes[i].path == m_currentThemePath) {
            currentIndex = i + 1;
        }
    }
    themeCombo->setCurrentIndex(currentIndex);

    themeLayout->addWidget(themeCombo, 1);
    layout->addLayout(themeLayout);

    // Deck preview - show sample cards
    QHBoxLayout* previewLayout = new QHBoxLayout;
    previewLayout->addStretch();

    // Create preview labels for sample cards
    QLabel* previewCard1 = new QLabel;
    QLabel* previewCard2 = new QLabel;
    QLabel* previewCard3 = new QLabel;
    QLabel* previewBack = new QLabel;

    previewCard1->setFixedSize(70, 100);
    previewCard2->setFixedSize(70, 100);
    previewCard3->setFixedSize(70, 100);
    previewBack->setFixedSize(70, 100);

    previewCard1->setFrameStyle(QFrame::Box);
    previewCard2->setFrameStyle(QFrame::Box);
    previewCard3->setFrameStyle(QFrame::Box);
    previewBack->setFrameStyle(QFrame::Box);

    previewLayout->addWidget(previewCard1);
    previewLayout->addWidget(previewCard2);
    previewLayout->addWidget(previewCard3);
    previewLayout->addWidget(previewBack);
    previewLayout->addStretch();

    layout->addLayout(previewLayout);

    // Function to update preview
    auto updatePreview = [&](const QString& themePath) {
        CardTheme previewTheme;
        if (themePath.isEmpty()) {
            previewTheme.loadBuiltinTheme();
        } else {
            previewTheme.loadTheme(themePath);
        }

        QSize previewSize(70, 100);

        // Show Ace of Spades, Queen of Hearts, King of Clubs, and card back
        Card aceSpades(Suit::Spades, Rank::Ace);
        Card queenHearts(Suit::Hearts, Rank::Queen);
        Card kingClubs(Suit::Clubs, Rank::King);

        QPixmap pix1 = previewTheme.cardFront(aceSpades, previewSize);
        QPixmap pix2 = previewTheme.cardFront(queenHearts, previewSize);
        QPixmap pix3 = previewTheme.cardFront(kingClubs, previewSize);
        QPixmap pixBack = previewTheme.cardBack(previewSize);

        previewCard1->setPixmap(pix1);
        previewCard2->setPixmap(pix2);
        previewCard3->setPixmap(pix3);
        previewBack->setPixmap(pixBack);
    };

    // Initialize preview with current theme
    updatePreview(themeCombo->currentData().toString());

    // Update preview when theme changes
    connect(themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int) {
        updatePreview(themeCombo->currentData().toString());
    });

    // Card size slider
    QHBoxLayout* scaleLayout = new QHBoxLayout;
    scaleLayout->addWidget(new QLabel(tr("Card Size:")));

    QSlider* scaleSlider = new QSlider(Qt::Horizontal);
    scaleSlider->setRange(50, 200);
    scaleSlider->setValue(static_cast<int>(m_cardScale * 100));
    scaleSlider->setTickPosition(QSlider::TicksBelow);
    scaleSlider->setTickInterval(25);

    QLabel* scaleLabel = new QLabel(QString("%1%").arg(static_cast<int>(m_cardScale * 100)));
    scaleLabel->setMinimumWidth(45);

    connect(scaleSlider, &QSlider::valueChanged, [scaleLabel](int value) {
        scaleLabel->setText(QString("%1%").arg(value));
    });

    scaleLayout->addWidget(scaleSlider, 1);
    scaleLayout->addWidget(scaleLabel);
    layout->addLayout(scaleLayout);

    // Sound checkbox
    QCheckBox* soundCheck = new QCheckBox(tr("Enable sound effects"));
    soundCheck->setChecked(m_soundEnabled);
    layout->addWidget(soundCheck);

    layout->addSpacing(10);

    // AI Difficulty
    QHBoxLayout* difficultyLayout = new QHBoxLayout;
    difficultyLayout->addWidget(new QLabel(tr("AI Difficulty:")));

    QComboBox* difficultyCombo = new QComboBox;
    difficultyCombo->addItem(tr("Easy"), static_cast<int>(AIDifficulty::Easy));
    difficultyCombo->addItem(tr("Medium"), static_cast<int>(AIDifficulty::Medium));
    difficultyCombo->addItem(tr("Hard"), static_cast<int>(AIDifficulty::Hard));
    difficultyCombo->setCurrentIndex(static_cast<int>(m_aiDifficulty));

    difficultyLayout->addWidget(difficultyCombo, 1);
    layout->addLayout(difficultyLayout);

    layout->addSpacing(15);

    // ===== GAME RULES SECTION =====
    QLabel* rulesHeader = new QLabel(tr("<b>Game Rules</b>"));
    layout->addWidget(rulesHeader);

    // End score
    QHBoxLayout* endScoreLayout = new QHBoxLayout;
    endScoreLayout->addWidget(new QLabel(tr("Game ends at score:")));
    QComboBox* endScoreCombo = new QComboBox;
    endScoreCombo->addItem("50", 50);
    endScoreCombo->addItem("75", 75);
    endScoreCombo->addItem("100 (Standard)", 100);
    endScoreCombo->addItem("150", 150);
    for (int i = 0; i < endScoreCombo->count(); ++i) {
        if (endScoreCombo->itemData(i).toInt() == m_gameRules.endScore) {
            endScoreCombo->setCurrentIndex(i);
            break;
        }
    }
    endScoreLayout->addWidget(endScoreCombo, 1);
    layout->addLayout(endScoreLayout);

    // Rule checkboxes
    QCheckBox* exactResetCheck = new QCheckBox(tr("Exactly %1 = reset to 50 (\"Save and take half\")").arg(m_gameRules.endScore));
    exactResetCheck->setChecked(m_gameRules.exactResetTo50);
    layout->addWidget(exactResetCheck);

    // Update the checkbox label when end score changes
    connect(endScoreCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [exactResetCheck, endScoreCombo](int) {
        int score = endScoreCombo->currentData().toInt();
        exactResetCheck->setText(QObject::tr("Exactly %1 = reset to 50 (\"Save and take half\")").arg(score));
    });

    QCheckBox* queenBreaksCheck = new QCheckBox(tr("Queen of Spades breaks hearts"));
    queenBreaksCheck->setChecked(m_gameRules.queenBreaksHearts);
    layout->addWidget(queenBreaksCheck);

    QCheckBox* moonChoiceCheck = new QCheckBox(tr("Shoot the Moon protection: if +26 to others would cause shooter to lose, they may take -26 instead"));
    moonChoiceCheck->setChecked(m_gameRules.moonProtection);
    layout->addWidget(moonChoiceCheck);

    QCheckBox* fullPolishCheck = new QCheckBox(tr("Full Polish: 99 points + takes 25 = reset to 98"));
    fullPolishCheck->setChecked(m_gameRules.fullPolish);
    layout->addWidget(fullPolishCheck);

    layout->addSpacing(10);

    // Info about themes
    QLabel* themeInfo = new QLabel(tr(
        "<small>Card themes are loaded from:<br>"
        "~/.local/share/carddecks/<br>"
        "/usr/share/carddecks/<br>"
        "Install KDE card decks for more themes.</small>"
    ));
    themeInfo->setWordWrap(true);
    layout->addWidget(themeInfo);

    layout->addStretch();

    // Buttons
    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel
    );
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() == QDialog::Accepted) {
        QString themePath = themeCombo->currentData().toString();
        if (themePath.isEmpty()) {
            m_theme->loadBuiltinTheme();
            m_currentThemePath.clear();
        } else {
            loadTheme(themePath);
        }
        m_gameView->setTheme(m_theme.get());

        m_cardScale = scaleSlider->value() / 100.0;
        m_gameView->setCardScale(m_cardScale);

        m_soundEnabled = soundCheck->isChecked();
        m_sound->setEnabled(m_soundEnabled);

        m_aiDifficulty = static_cast<AIDifficulty>(difficultyCombo->currentData().toInt());
        m_game->setAIDifficulty(m_aiDifficulty);

        // Update game rules
        m_gameRules.endScore = endScoreCombo->currentData().toInt();
        m_gameRules.exactResetTo50 = exactResetCheck->isChecked();
        m_gameRules.queenBreaksHearts = queenBreaksCheck->isChecked();
        m_gameRules.moonProtection = moonChoiceCheck->isChecked();
        m_gameRules.fullPolish = fullPolishCheck->isChecked();
        m_game->setRules(m_gameRules);
    }
}

void MainWindow::showAbout() {
    QMessageBox::about(this, tr("About Hearts"),
        tr("<h2>Hearts</h2>"
           "<p>A classic card game for Qt.</p>"
           "<p>Try to avoid taking hearts and especially the Queen of Spades!</p>"
           "<p><b>Rules:</b></p>"
           "<ul>"
           "<li>Each heart is worth 1 point</li>"
           "<li>Queen of Spades is worth 13 points</li>"
           "<li>Lowest score wins</li>"
           "<li>\"Shoot the Moon\" - take all hearts and QoS to give 26 points to others</li>"
           "</ul>"
           "<p>Version 1.0</p>"));
}

void MainWindow::showScores() {
    if (!m_game) return;

    QString text = "<table cellpadding='5'>"
                   "<tr><th>Player</th><th>Score</th></tr>";

    for (int i = 0; i < 4; ++i) {
        const Player* p = m_game->player(i);
        text += QString("<tr><td>%1</td><td align='right'>%2</td></tr>")
                .arg(p->name())
                .arg(p->totalScore());
    }
    text += "</table>";

    QMessageBox msg(this);
    msg.setWindowTitle(tr("Scores"));
    msg.setTextFormat(Qt::RichText);
    msg.setText(text);
    msg.exec();
}

void MainWindow::showStatistics() {
    double avgScore = m_gamesPlayed > 0 ? static_cast<double>(m_totalScore) / m_gamesPlayed : 0;
    double winRate = m_gamesPlayed > 0 ? 100.0 * m_gamesWon / m_gamesPlayed : 0;

    QString text = tr(
        "<h3>Lifetime Statistics</h3>"
        "<table cellpadding='5'>"
        "<tr><td>Games Played:</td><td align='right'><b>%1</b></td></tr>"
        "<tr><td>Games Won:</td><td align='right'><b>%2</b></td></tr>"
        "<tr><td>Win Rate:</td><td align='right'><b>%3%</b></td></tr>"
        "<tr><td>Average Score:</td><td align='right'><b>%4</b></td></tr>"
        "<tr><td>Best Score:</td><td align='right'><b>%5</b></td></tr>"
        "<tr><td>Shot the Moon:</td><td align='right'><b>%6</b></td></tr>"
        "</table>"
    ).arg(m_gamesPlayed)
     .arg(m_gamesWon)
     .arg(winRate, 0, 'f', 1)
     .arg(avgScore, 0, 'f', 1)
     .arg(m_bestScore == 999 ? QString("-") : QString::number(m_bestScore))
     .arg(m_shootTheMoonCount);

    QMessageBox msg(this);
    msg.setWindowTitle(tr("Statistics"));
    msg.setTextFormat(Qt::RichText);
    msg.setText(text);

    QPushButton* resetBtn = msg.addButton(tr("Reset"), QMessageBox::DestructiveRole);
    msg.addButton(QMessageBox::Ok);
    msg.exec();

    if (msg.clickedButton() == resetBtn) {
        if (QMessageBox::question(this, tr("Reset Statistics"),
                tr("Are you sure you want to reset all statistics?")) == QMessageBox::Yes) {
            m_gamesPlayed = 0;
            m_gamesWon = 0;
            m_totalScore = 0;
            m_bestScore = 999;
            m_shootTheMoonCount = 0;
            saveSettings();
        }
    }
}

void MainWindow::toggleFullscreen() {
    if (isFullScreen()) {
        showNormal();
        m_fullscreenAction->setChecked(false);
    } else {
        showFullScreen();
        m_fullscreenAction->setChecked(true);
    }
}

void MainWindow::toggleMenuBar() {
    bool visible = m_hideMenuAction->isChecked();
    menuBar()->setVisible(visible);
}

void MainWindow::onGameEnded(int winner) {
    m_gamesPlayed++;

    int myScore = m_game->player(0)->totalScore();
    m_totalScore += myScore;

    if (myScore < m_bestScore) {
        m_bestScore = myScore;
    }

    if (winner == 0) {
        m_gamesWon++;
    }

    // Check if player shot the moon in any round (they'd have 0 round score after shooting)
    // This is approximate - would need proper tracking in Game class for accuracy

    saveSettings();
}

void MainWindow::undo() {
    if (m_game && m_game->canUndo()) {
        m_game->undo();
    }
}

void MainWindow::onUndoAvailableChanged(bool available) {
    m_undoAction->setEnabled(available);
}
