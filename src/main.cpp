#include "gamebridge.h"
#include "cardimageprovider.h"
#include <QApplication>
#include <QMainWindow>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQuickStyle>
#include <QMenuBar>
#include <QSurfaceFormat>
#include <QIcon>
#include <QKeyEvent>
#include <QTimer>

// Intercept Alt key to show the hidden menu bar
class Application : public QApplication {
public:
    using QApplication::QApplication;

    void setMenuBarActivation(QMenuBar* bar) {
        m_bar = bar;
    }

    bool notify(QObject* receiver, QEvent* event) override {
        if (event->type() == QEvent::KeyPress) {
            auto* ke = static_cast<QKeyEvent*>(event);
            if (ke->key() == Qt::Key_Alt && !ke->isAutoRepeat())
                m_altOnly = true;
            else
                m_altOnly = false;
        } else if (event->type() == QEvent::KeyRelease) {
            auto* ke = static_cast<QKeyEvent*>(event);
            if (ke->key() == Qt::Key_Alt && !ke->isAutoRepeat() && m_altOnly) {
                m_altOnly = false;
                if (m_bar && !m_bar->isVisible()) {
                    m_bar->show();
                    if (!m_bar->actions().isEmpty())
                        m_bar->setActiveAction(m_bar->actions().first());
                }
            }
        }
        return QApplication::notify(receiver, event);
    }

private:
    QMenuBar* m_bar = nullptr;
    bool m_altOnly = false;
};

int main(int argc, char* argv[]) {
    // MSAA for smooth card edges
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setSamples(4);
    QSurfaceFormat::setDefaultFormat(format);

    Application app(argc, argv);

    app.setApplicationName("Hearts");
    app.setApplicationVersion("1.0.4");
    app.setOrganizationName("Hearts");
    app.setWindowIcon(QIcon::fromTheme("qt-hearts", QIcon(":/data/icons/qt-hearts.svg")));

    // Respect user/desktop style if set, otherwise use Fusion
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle("Fusion");
    }

    GameBridge* gameBridge = new GameBridge();

    QMainWindow mainWindow;
    mainWindow.setWindowTitle("Hearts");
    mainWindow.setMinimumSize(800, 600);
    mainWindow.resize(1024, 768);

    QQuickWidget* quickWidget = new QQuickWidget(&mainWindow);
    quickWidget->setFormat(format);
    quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    quickWidget->engine()->addImageProvider("cards", new CardImageProvider(gameBridge->theme()));
    quickWidget->engine()->addImageProvider("cardpreview", new CardImageProvider(gameBridge->previewTheme()));
    quickWidget->rootContext()->setContextProperty("gameBridge", gameBridge);

    quickWidget->setSource(QUrl("qrc:/qml/Main.qml"));

    mainWindow.setCentralWidget(quickWidget);

    // ===== Native Menu Bar =====
    QMenuBar* menuBar = mainWindow.menuBar();

    // Game menu
    QMenu* gameMenu = menuBar->addMenu(QObject::tr("&Game"));
    QAction* newGameAction = gameMenu->addAction(QObject::tr("&New Game"), gameBridge, &GameBridge::newGame);
    newGameAction->setShortcut(QKeySequence("Ctrl+N"));
    gameMenu->addSeparator();
    QAction* undoAction = gameMenu->addAction(QObject::tr("&Undo"), gameBridge, &GameBridge::undo);
    undoAction->setShortcut(QKeySequence("Ctrl+Z"));
    QObject::connect(gameBridge, &GameBridge::undoAvailableChanged, undoAction, [undoAction, gameBridge]() {
        undoAction->setEnabled(gameBridge->undoAvailable());
    });
    undoAction->setEnabled(gameBridge->undoAvailable());
    gameMenu->addSeparator();
    QAction* quitAction = gameMenu->addAction(QObject::tr("&Quit"), gameBridge, &GameBridge::quit);
    quitAction->setShortcut(QKeySequence("Ctrl+Q"));

    // View menu
    QMenu* viewMenu = menuBar->addMenu(QObject::tr("&View"));
    viewMenu->addAction(QObject::tr("&Scores..."), gameBridge, &GameBridge::openScoresRequested);
    viewMenu->addAction(QObject::tr("S&tatistics..."), gameBridge, &GameBridge::openStatisticsRequested);
    viewMenu->addSeparator();
    QAction* fullscreenAction = viewMenu->addAction(QObject::tr("&Fullscreen"));
    fullscreenAction->setShortcut(QKeySequence("F11"));
    fullscreenAction->setCheckable(true);
    QObject::connect(fullscreenAction, &QAction::toggled, &mainWindow, [&mainWindow](bool checked) {
        if (checked) {
            mainWindow.showFullScreen();
        } else {
            mainWindow.showNormal();
        }
    });
    viewMenu->addSeparator();
    QAction* menuBarAction = viewMenu->addAction(QObject::tr("Show &Menu Bar"));
    menuBarAction->setShortcut(QKeySequence("Ctrl+M"));
    menuBarAction->setCheckable(true);
    menuBarAction->setChecked(gameBridge->showMenuBar());
    QObject::connect(menuBarAction, &QAction::toggled, gameBridge, [gameBridge](bool checked) {
        gameBridge->setShowMenuBar(checked);
    });
    QObject::connect(gameBridge, &GameBridge::showMenuBarChanged, menuBarAction, [menuBarAction, gameBridge]() {
        menuBarAction->setChecked(gameBridge->showMenuBar());
    });

    // Settings menu
    QMenu* settingsMenu = menuBar->addMenu(QObject::tr("&Settings"));
    settingsMenu->addAction(QObject::tr("&Preferences..."), gameBridge, &GameBridge::openSettingsRequested);

    // Help menu
    QMenu* helpMenu = menuBar->addMenu(QObject::tr("&Help"));
    helpMenu->addAction(QObject::tr("&About Hearts..."), gameBridge, &GameBridge::openAboutRequested);

    // Menu bar visibility
    menuBar->setVisible(gameBridge->showMenuBar());
    QObject::connect(gameBridge, &GameBridge::showMenuBarChanged, menuBar, [menuBar, gameBridge]() {
        menuBar->setVisible(gameBridge->showMenuBar());
    });

    // Add all shortcut actions to the window so they work when menu bar is hidden
    mainWindow.addAction(newGameAction);
    mainWindow.addAction(undoAction);
    mainWindow.addAction(quitAction);
    mainWindow.addAction(fullscreenAction);
    mainWindow.addAction(menuBarAction);

    app.setMenuBarActivation(menuBar);

    // Re-hide menu bar after Alt-activated menu interaction
    auto hideMenuIfNeeded = [menuBar, gameBridge]() {
        if (!gameBridge->showMenuBar()) {
            QTimer::singleShot(100, menuBar, [menuBar, gameBridge]() {
                if (!gameBridge->showMenuBar() && !menuBar->activeAction())
                    menuBar->setVisible(false);
            });
        }
    };
    QObject::connect(gameMenu, &QMenu::aboutToHide, hideMenuIfNeeded);
    QObject::connect(viewMenu, &QMenu::aboutToHide, hideMenuIfNeeded);
    QObject::connect(settingsMenu, &QMenu::aboutToHide, hideMenuIfNeeded);
    QObject::connect(helpMenu, &QMenu::aboutToHide, hideMenuIfNeeded);

    mainWindow.show();

    return app.exec();
}
